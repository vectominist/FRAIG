/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include <iomanip>
#include <unordered_map>
#include <algorithm>
#include <utility>
#include <cmath>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/
#define MAXGATE 1000005
Var gateMap[MAXGATE];
vector<GatePair>  gateMergeList;

vector<CirGate*> gateTmp(MAXGATE, 0);
size_t gateNum;

SimValue SV0(0);

inline int breakLim(const int x){
  // y = sqrt(5x) + 10
  return (int)ceil(sqrt(5.0 * (double)x) + 10.0);
}

inline int proveLim(const int x){
  // y = -3x + 10
  return (-3) * x + 10;
}

unsigned _setW;

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed



void
CirMgr::strash()
{
  bool _merged = false;

  UMap _hmap;

  for(size_t i = 0; i < _netList.size(); ++i){
    if(_netList[i] == 0) continue;
    if(_netList[i]->isAig()){
      HashKey nowKey((size_t)(_netList[i]->getFaninList(0)), _netList[i]->getInvertedList(0), 
        (size_t)(_netList[i]->getFaninList(1)), _netList[i]->getInvertedList(1));

      UMap::iterator it = _hmap.find(nowKey);
      if(it != _hmap.end()){
        // Found!
        // -> Merge _netList[i] to it->second
        cout << "Strashing: " << ((CirGate*)(it->second))->getGateIndex() << " merging " << _netList[i]->getGateIndex() << "..." << endl;
        mergeGate((CirGate*)(it->second), _netList[i], false);
        deleteGate(_netList[i]);

        if(!_merged) _merged = true;
      } else {
        _hmap.insert(make_pair(nowKey, (size_t)_netList[i]));
      }
    }
  }


  if(_merged){
    buildNetlist();
    updateFloating();
  }
}

void
CirMgr::fraig()
{
  if(_fecGroupList.size() == 0) return;
  gateMergeList.clear();

  SatSolver _solver;
  size_t nowFECGsize = _fecGroupList.size();
  /*size_t proveLimit = 10, growthL = 5;
  size_t breakLimit = 50, descentL = 3;*/
  int cntF = 0;
  while(!_fecGroupList.empty() || !gateMergeList.empty()){
    size_t provenSAT = 0;
    
    // 20190207 changed
    if(!gateMergeList.empty()){
      // Merge all gates in gateMergeList
      cout << endl;
      size_t provenUNSAT = MergeAll();
      nowFECGsize -= provenUNSAT;
      cout << "Updating by UNSAT... Total #FEC group = " << _fecGroupList.size() << endl;
      updateFloating();
    }

    if(!_fecGroupList.empty()){
      // Prove small cases
      provenSAT = proveSmall(_solver);
      if(provenSAT > 0){
        nowFECGsize -= provenSAT;
        cout << "Updating by SAT... Total #FEC group = " << _fecGroupList.size() << endl;
        // Simulate again...
        reSimulate();
      }
    }

    // 20190207 changed
    if(!gateMergeList.empty()){
      // Merge all gates in gateMergeList
      size_t provenUNSAT = MergeAll();
      nowFECGsize -= provenUNSAT;
      cout << "Updating by UNSAT... Total #FEC group = " << _fecGroupList.size() << endl;
      updateFloating();
    }

    if(!_fecGroupList.empty()){
      // Prove small cases 
      provenSAT = proveSmall3(_solver);
      if(provenSAT > 0){
        nowFECGsize -= provenSAT;
        cout << "Updating by SAT... Total #FEC group = " << _fecGroupList.size() << endl;
        // Simulate again...
        reSimulate();
      }
    }

    if(!gateMergeList.empty()){
      // Merge all gates in gateMergeList
      size_t provenUNSAT = MergeAll();
      nowFECGsize -= provenUNSAT;
      cout << "Updating by UNSAT... Total #FEC group = " << _fecGroupList.size() << endl;
      updateFloating();
    }

    if(!_fecGroupList.empty()){
      // -> Prove larger FEC groups
      provenSAT = proveLarge(_solver, proveLim(cntF), breakLim(cntF));
      if(provenSAT > 0){
        nowFECGsize -= provenSAT;
        cout << "Updating by SAT... Total #FEC group = " << _fecGroupList.size() << endl;
        reSimulate();
      }
      /*proveLimit += growthL;
      breakLimit -= descentL;*/
    }
    ++cntF;
    // clearNowPackList();
  }
  buildNetlist();
  updateFloating();
  strash();
  _fraiged = true;
}

void 
CirMgr::printFEC(size_t id) const{
  if(_fecGroupList.empty()) return;
  if(_totalList[id] == 0) return;

  for(size_t i = 0; i < _fecGroupList.size(); ++i){
    if(_fecGroupList[i]->count(_totalList[id]) > 0){
      FECgroup::iterator it = _fecGroupList[i]->begin();
      SimValue tmpSV = (*it)->getSimValue();
      for( ; it != _fecGroupList[i]->end(); ++it){
        if((*it) == _totalList[id])continue;
        cout << " ";
        if(tmpSV != (*it)->getSimValue()) cout << "!";
        cout << (*it)->getGateIndex();
      }
    }
  }
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/

void CirMgr::mergeGate(CirGate* tarGate, CirGate* uGate, const bool _inv){
  assert(tarGate != 0);
  assert(uGate != 0);

  // Merge uGate to tarGate
  // uGate   : unwanted gate
  // tarGate : target gate
  // _inv    : inverted or not

  for(size_t i = 0; i < uGate->getFaninSize(); ++i){
    assert(uGate->getFaninList(i)->removeFanoutGate(uGate));
  }

  for(size_t i = 0; i < uGate->getFanoutSize(); ++i){
    tarGate->addFanout(uGate->getFanoutList(i));

    for(size_t j = 0; j < uGate->getFanoutList(i)->getFaninSize(); ++j){
      if(uGate->getFanoutList(i)->getFaninList(j) == uGate){
        uGate->getFanoutList(i)->modifyFanin(tarGate, j);
        if(/*uGate->getFanoutList(i)->getInvertedList(j) && */_inv){
          uGate->getFanoutList(i)->modifyInvert((uGate->getFanoutList(i)->getInvertedList(j)) ? false : true, j);
        }
      }
    }
  }
}

void 
CirMgr::genProofModel(SatSolver& s){
  // Generates proof model
  // Map between gates and var : gateMap
  //   Using the gate index to be the index of the map (array)

  // bool FST = false;
  Var dummyG = s.newVar();
  gateMap[0] = s.newVar();
  s.addAigCNF(gateMap[0], dummyG, false, dummyG, true);
  for(size_t i = 1; i <= _M; ++i){
    if(_totalList[i] != 0 && !isFloating(_totalList[i])){
      gateMap[_totalList[i]->getGateIndex()] = s.newVar();
      /*if(!FST){
        FST = true;
        cerr << "First newVar : " << gateMap[_totalList[i]->getGateIndex()] << endl;
      }*/
    }
  }

  for(size_t i = 0; i < _netList.size(); ++i){
    if(_netList[i]->isAig()){
      s.addAigCNF(gateMap[_netList[i]->getGateIndex()], 
                  gateMap[_netList[i]->getFaninList(0)->getGateIndex()], _netList[i]->getInvertedList(0), 
                  gateMap[_netList[i]->getFaninList(1)->getGateIndex()], _netList[i]->getInvertedList(1));
    }
  }
}

size_t 
CirMgr::proveSmall(SatSolver& s){
  s.initialize();
  genProofModel(s);
  size_t _provenSAT = 0;
  cerr << "Proving small..." << endl;
  // Prove small cases : cases with gates = 2
  for(int i = 0; i < (int)_fecGroupList.size(); ++i){
    if(_fecGroupList[i]->size() > 2) continue;
    // assert(_fecGroupList[i]->size() == 2);
    FECgroup::iterator it = _fecGroupList[i]->begin();

    CirGate* _G1 = *it;   ++it;
    CirGate* _G2 = *it;

    if(isFloating(_G1) || isFloating(_G2)){
      // If is floating : doesn't need to prove
      _fecGroupList[i]->clear();
      delete _fecGroupList[i];

      if(i == (int)_fecGroupList.size() - 1){
            _fecGroupList.pop_back();
      } else {
        _fecGroupList[i] = _fecGroupList.back();
        _fecGroupList.pop_back();
        --i;
      }
      continue;
    }

    // cout << "small : fecGroupList[" << i << "]... ";
    bool result;
    //Var _g1, _g2;
    if(_G1->getGateIndex() == 0){
      // const 0
      //_g1 = gateMap[_G2->getGateIndex()];
      result = proveZero(s, _G2, SV0 != _G2->getSimValue());
    } else {
      //_g1 = gateMap[_G1->getGateIndex()];
      SimValue _tmpSV = _G1->getSimValue();
      //_g2 = gateMap[_G2->getGateIndex()];

      result = proveGates(s, _G1, _G2, _tmpSV != _G2->getSimValue());
    }

    if(result){
      // SAT -> _g1 != _g2
      ++_provenSAT;
    } else {
      // UNSAT -> _g1 == _g2 -> Merge!
      gateMergeList.push_back(make_pair(_G1, _G2));
    }
    cout << flush;

    _fecGroupList[i]->clear();
    delete _fecGroupList[i];

    if(i == (int)_fecGroupList.size() - 1){
      _fecGroupList.pop_back();
    } else {
      _fecGroupList[i] = _fecGroupList.back();
      _fecGroupList.pop_back();
      --i;
    }
  }
  if(_provenSAT > 0)cout << endl;
  return _provenSAT;
}

size_t 
CirMgr::proveSmall3(SatSolver& s){
  s.initialize();
  genProofModel(s);
  size_t _provenSAT = 0;
  cerr << "Proving small 3..." << endl;
  // Prove small cases : cases with gates = 3
  for(int i = 0; i < (int)_fecGroupList.size(); ++i){
    if(_fecGroupList[i]->size() != 3) continue;
    // assert(_fecGroupList[i]->size() == 2);
    FECgroup::iterator it = _fecGroupList[i]->begin();

    CirGate* _G1 = *it;   ++it;
    CirGate* _G2 = *it;   ++it;
    CirGate* _G3 = *it;

    if(isFloating(_G1)){
      _fecGroupList[i]->clear();
      delete _fecGroupList[i];

      _fecGroupList[i] = new FECgroup;
      _fecGroupList[i]->insert(_G2);
      _fecGroupList[i]->insert(_G3);
      continue;
    }
    if(isFloating(_G2)){
      _fecGroupList[i]->clear();
      delete _fecGroupList[i];

      _fecGroupList[i] = new FECgroup;
      _fecGroupList[i]->insert(_G1);
      _fecGroupList[i]->insert(_G3);
      continue;
    }
    if(isFloating(_G3)){
      _fecGroupList[i]->clear();
      delete _fecGroupList[i];

      _fecGroupList[i] = new FECgroup;
      _fecGroupList[i]->insert(_G1);
      _fecGroupList[i]->insert(_G2);
      continue;
    }
    // cout << "small3 : fecGroupList[" << i << "]... ";
    bool result1, result2, result3;
    //Var _g1, _g2, _g3;
    if(_G1->getGateIndex() == 0){
      // const 0
      //_g2 = gateMap[_G2->getGateIndex()];
      
      SimValue _tmpSV2 = _G2->getSimValue();
      SimValue _tmpSV3 = _G3->getSimValue();
      result1 = proveZero(s, _G2, SV0 != _tmpSV2);
      result2 = proveGates(s, _G2, _G3, _tmpSV2 != _tmpSV3);
      result3 = proveZero(s, _G3, SV0 != _tmpSV3);
    } else {
      /*_g1 = gateMap[_G1->getGateIndex()];
      _g2 = gateMap[_G2->getGateIndex()];
      _g3 = gateMap[_G3->getGateIndex()];*/
      SimValue _tmpSV1 = _G1->getSimValue();
      SimValue _tmpSV2 = _G2->getSimValue();
      SimValue _tmpSV3 = _G3->getSimValue();

      result1 = proveGates(s, _G1, _G2, _tmpSV1 != _tmpSV2);
      result2 = proveGates(s, _G2, _G3, _tmpSV2 != _tmpSV3);
      result3 = proveGates(s, _G3, _G1, _tmpSV3 != _tmpSV1);
    }

    if      (result1  &&  result2  &&  result3){
      _provenSAT += 3;
    }
    else if(!result1  &&  result2  &&  result3){
      gateMergeList.push_back(make_pair(_G1, _G2));
      _provenSAT += 2;
    }
    else if( result1  && !result2  &&  result3){
      gateMergeList.push_back(make_pair(_G2, _G3));
      _provenSAT += 2;
    }
    else if( result1  &&  result2  && !result3){
      gateMergeList.push_back(make_pair(_G3, _G1));
      _provenSAT += 2;
    }
    else if(!result1  && !result2  &&  result3){
      // No such situation
    }
    else if( result1  && !result2  && !result3){
      // No such situation
    }
    else if(!result1  &&  result2  && !result3){
      // No such situation
    }
    else if(!result1  && !result2  && !result3){
      if(_G1->getTopOrder() < _G2->getTopOrder() && _G1->getTopOrder() < _G3->getTopOrder()){
        gateMergeList.push_back(make_pair(_G1, _G2));
        gateMergeList.push_back(make_pair(_G1, _G3));
      }
      else if(_G2->getTopOrder() < _G1->getTopOrder() && _G2->getTopOrder() < _G3->getTopOrder()){
        gateMergeList.push_back(make_pair(_G2, _G1));
        gateMergeList.push_back(make_pair(_G2, _G3));
      }
      else if(_G3->getTopOrder() < _G1->getTopOrder() && _G3->getTopOrder() < _G2->getTopOrder()){
        gateMergeList.push_back(make_pair(_G3, _G1));
        gateMergeList.push_back(make_pair(_G3, _G2));
      }
    }
    cout << flush;

    _fecGroupList[i]->clear();
    delete _fecGroupList[i];

    if(i == (int)_fecGroupList.size() - 1){
      _fecGroupList.pop_back();
    } else {
      _fecGroupList[i] = _fecGroupList.back();
      _fecGroupList.pop_back();
      --i;
    }
  }
  if(_provenSAT > 0)cout << endl;
  return _provenSAT;
}

size_t 
CirMgr::proveLarge(SatSolver& s, const size_t _lim1, const size_t _lim2){
  cerr << "Proving large..." << endl;
  s.initialize();
  genProofModel(s);
  size_t _provenSAT = 0;
  //cerr << "_lim1 = " << _lim1 << " _lim2 = " << _lim2 << endl;
  // Prove larger cases : cases with gates <= _lim1
  for(int i = 0; i < (int)_fecGroupList.size(); ++i){
    // cerr << "large : fecGroupList[" << i << "] (" << _fecGroupList[i]->size() << ")... "; 
    // copy all the gates from _fecGroupList[i] to gateTmp (set -> array)
    FECgroup::iterator it = _fecGroupList[i]->begin();
    if((*it)->getGateIndex() != 0){ // NOT CONST0
      // If this group contains CONST0 -> prove all 
      if(_fecGroupList[i]->size() > _lim1) continue;
      if(_fecGroupList[i]->size() > _lim2) {
        // If more than _lim2 gates -> delete
        _fecGroupList[i]->clear();
        delete _fecGroupList[i];
        _fecGroupList[i] = 0;

        if(i == (int)_fecGroupList.size() - 1){
          _fecGroupList.pop_back();
        } else {
          _fecGroupList[i] = _fecGroupList.back();
          _fecGroupList.pop_back();
          --i;
        }
        continue;
      }
    }

    // 20190207 updated
    /*if((*it)->getGateIndex() == 0 && _fraiged){
      _fecGroupList[i]->clear();
      delete _fecGroupList[i];
      _fecGroupList[i] = 0;

      if(i == (int)_fecGroupList.size() - 1){
        _fecGroupList.pop_back();
      } else {
        _fecGroupList[i] = _fecGroupList.back();
        _fecGroupList.pop_back();
        --i;
      }
      continue;
    }*/
    // 

    gateNum = _fecGroupList[i]->size();
    for(size_t j = 0; it != _fecGroupList[i]->end(); ++it, ++j){
      gateTmp[j] = *it;
    }
    
    _fecGroupList[i]->clear();
    delete _fecGroupList[i];
    _fecGroupList[i] = 0;

    // Prove all pairs
    bool check0 = false;
    for(size_t j = 0; j < 2; ++j){
      if(gateTmp[j] == 0) continue;
      if(isFloating(gateTmp[j])){
        gateTmp[j] = 0;
        continue;
      }
      for(size_t k = j + 1; k < gateNum; ++k){
        if(gateTmp[k] == 0) continue;
        if(isFloating(gateTmp[k])){
          gateTmp[k] = 0;
          continue;
        }
        
        if(gateTmp[j]->getGateIndex() == 0){
          // CONST 0
          check0 = true;
          bool result = proveZero(s, gateTmp[k], SV0 != gateTmp[k]->getSimValue());
          if(result){
            // SAT -> g[j] != g[k]
            // Fucking do nothing?
            ++_provenSAT;
          } else {
            // UNSAT -> g[j] == g[k]
            gateMergeList.push_back(make_pair(gateTmp[j], gateTmp[k]));
            /*gateTmp[j] =*/ gateTmp[k] = 0;
            //break;
          }
        } else {
          bool result = proveGates(s, gateTmp[j], gateTmp[k],
            gateTmp[j]->getSimValue() != gateTmp[k]->getSimValue());
          if(result){
            // SAT -> g[j] != g[k]
            // Fucking do nothing?
            ++_provenSAT;
          } else {
            // UNSAT -> g[j] == g[k]
            gateMergeList.push_back(make_pair(gateTmp[j], gateTmp[k]));
            gateTmp[j] = gateTmp[k] = 0;
            break;
          }
        }
        cout << flush;
        if(k == gateNum - 1){
          gateTmp[j] = 0;
          // gateTmp[j] != other gates
        }
      }
      if(check0) break;
    }

    FECgroup* newGroup = new FECgroup;
    for(size_t j = 0; j < gateNum; ++j){
      if(gateTmp[j] != 0) newGroup->insert(gateTmp[j]);
    }
    if(newGroup->size() < 2){
      newGroup->clear();
      delete newGroup;
      if(i == (int)_fecGroupList.size() - 1){
        _fecGroupList.pop_back();
      } else {
        _fecGroupList[i] = _fecGroupList.back();
        _fecGroupList.pop_back();
        --i;
      }
    }
    else {
      _fecGroupList[i] = newGroup;
    }
  }
  // _fecGroupList.clear();
  /*if(_fecGroupList.size() == 1){
    _fecGroupList[0]->clear();
    delete _fecGroupList[0];
    _fecGroupList.clear();
  }*/
  if(_provenSAT > 0)cout << endl;
  return _provenSAT;
}

inline bool 
CirMgr::isFloating(CirGate* _g) const{
  // Only 2 ?
  return /*(_floatList1.count(_g) > 0) || */(_floatList2.count(_g) > 0);
}

bool 
CirMgr::proveZero(SatSolver& s, CirGate* _g, const bool _inv){
  // Proving gate _g == CONST0
  s.assumeRelease();
  s.assumeProperty(gateMap[_g->getGateIndex()], _inv ? false : true);
  if(s.assumpSolve()){
    // true -> SAT -> _g != 0
    collectSimValue(s);
    cout << "\r                           \r";
    cout << "(" << setw(7) << 0 << " != ";
    
    if(_g->getSimValue() != SV0){
      cout << "!";
      _setW = 6;
    } else _setW = 7;
    cout << setw(_setW) << _g->getGateIndex() << ")... ";

    return true;
  } else {
    // false -> UNSAT -> _g1 == 0
    cout << "\r                           \r";
    cout << "(" << setw(7) << 0 << " == ";
    if(_g->getSimValue() != SV0){
      cout << "!";
      _setW = 6;
    } else _setW = 7;
    cout << setw(_setW) << _g->getGateIndex() << ")... ";

    return false;
  }
}

bool 
CirMgr::proveGates(SatSolver& s, CirGate* _g1, CirGate* _g2, const bool _inv){
  // Proving gate _g1 == _g2
  // newV = _g1 ^ _g2
  // If SAT   : exists a pattern such that makes newV = 1 -> _g1 != _g2
  // If UNSAT : _g1 == _g2
  Var newV = s.newVar();
  s.addXorCNF(newV, gateMap[_g1->getGateIndex()], false, gateMap[_g2->getGateIndex()], _inv);
  s.assumeRelease();
  s.assumeProperty(newV, true);

  if(s.assumpSolve()){
    // true -> SAT -> _g1 != _g2
    collectSimValue(s);
    cout << "\r                           \r";
    cout << "(" << setw(7) << _g1->getGateIndex() << " != ";
    if(_g1->getSimValue() != _g2->getSimValue()){
      cout << "!";
      _setW = 6;
    } else _setW = 7;
    cout << setw(_setW) << _g2->getGateIndex() << ")... ";

    return true;
  } else {
    // false -> UNSAT -> _g1 == _g2
    cout << "\r                           \r";
    cout << "(" << setw(7) << _g1->getGateIndex() << " == ";
    if(_g1->getSimValue() != _g2->getSimValue()){
      cout << "!";
      _setW = 6;
    } else _setW = 7;
    cout << setw(_setW) << _g2->getGateIndex() << ")... ";
    return false;
  }
}

void 
CirMgr::collectSimValue(SatSolver& s){
  // Collect SimValue from SatSolver
  // Only when SAT has occurred

  if((_packs & 63) == 0){
    SimValuePack* newPack = new SimValuePack;
    _nowPackList.push_back(newPack);
    newPack->clear();
    for(size_t i = 0; i < _I; ++i){
      newPack->push_back(SimValue());
    }
  }

  for(size_t i = 0; i < _I; ++i){
    if(s.getValue(gateMap[_PIList[i]->getGateIndex()]) == 1){
      (*_nowPackList.back())[i].modify((_packs & 63), true);
    }
  }
  ++_packs;
}

void 
CirMgr::reSimulate(){
  if(_fecGroupList.empty()) return;
  cerr << "reSimulating..." << endl;
  for(size_t i = 0; i < _nowPackList.size(); ++i){
    SimulateList(i);
    updateFECgroups();
  }
  // clearNowPackList();
  recycleNowPackList();
}

size_t 
CirMgr::MergeAll(){
  for(size_t i = 0; i < gateMergeList.size(); ++i){
    bool inv = gateMergeList[i].first->getSimValue() != gateMergeList[i].second->getSimValue();
    if(gateMergeList[i].first->getTopOrder() < gateMergeList[i].second->getTopOrder()){
      // Merge the larger topOrder to the smaller one
      mergeGate(gateMergeList[i].first, gateMergeList[i].second, inv);
      fraigMessage(gateMergeList[i].first->getGateIndex(), inv, gateMergeList[i].second->getGateIndex());

      deleteGate(gateMergeList[i].second);
    } else {
      mergeGate(gateMergeList[i].second, gateMergeList[i].first, inv);
      fraigMessage(gateMergeList[i].second->getGateIndex(), inv, gateMergeList[i].first->getGateIndex());

      deleteGate(gateMergeList[i].first);
    }
  }

  size_t _proven = gateMergeList.size();
  gateMergeList.clear();
  buildNetlist();
  updateFloating();
  // printNetlist();
  return _proven;
}

void 
CirMgr::fraigMessage(unsigned newID, bool newInv, unsigned oldID) const{
  cout << "Fraig: " << newID << " merging ";
  if(newInv == true) cout << "!";
  cout << oldID << "..." << endl;
}