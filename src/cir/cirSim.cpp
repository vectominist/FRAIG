/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <algorithm>
#include <utility>
#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/
FECgroupS tmpList;
RandomNumGen rG(0);
const int rLIM = 2147483647;
int ABS(int x){ return (x > 0) ? x : -x;}

bool FECgroupCmp(FECgroup* _g1, FECgroup* _g2){
   FECgroup::iterator it1 = _g1->begin();
   FECgroup::iterator it2 = _g2->begin();

   return ((*it1)->getGateIndex()) < ((*it2)->getGateIndex());
}

class fecCounter{
public:
  fecCounter() { _sz[0] = _sz[1] = _sz[2] = _sz[3] = _sz[4] = 0; }
  fecCounter(const int _initN) { _sz[0] = _sz[1] = _sz[2] = _sz[3] = 0; _sz[4] = _initN; }
  ~fecCounter() {}

  void initialize(const int _initN) { _sz[0] = _sz[1] = _sz[2] = _sz[3] = 0; _sz[4] = _initN; }
  void add(const int _newD){
    _sz[0] = _sz[1]; _sz[1] = _sz[2]; _sz[2] = _sz[3]; _sz[3] = _sz[4]; _sz[4] = _newD;
  }
  bool stop() const {
    if(ABS(_sz[0] - _sz[4]) <= 5) return true;
    return false;
  }
private:
  int _sz[5];
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
  size_t psize = 0; // number of patterns
  // A & 63 == A % 64
  if(_fecGroupList.empty()){
    buildFECgroupList();
  }
  if(_fecGroupList.empty()){
    cout << psize << " patterns simulated." << endl;
    return;
  }

  if(_nowPack != 0){
    delete _nowPack;
    _nowPack = 0;
  }
  
  if(_I <= ILimit){
    SVgenerator_all();
    psize = ONE << _I;
    for(size_t i = 0; i < _nowPackList.size(); ++i){
      _nowPack = _nowPackList[i];
      SimulateAll();
      updateFECgroups();
      reportSimLog((psize >= 64) ? 64 : psize);
    }
    
  } else {
    fecCounter _fC(1000000);

    // 20190207 updated
    if(!_goodPackList.empty()){
      psize += _goodPackList.size() << 6;
      for(size_t i = 0; i < _goodPackList.size(); ++i){
        _nowPack = _goodPackList[i];
        SimulateAll();
        updateFECgroups();
        // reportSimLog((psize >= 64) ? 64 : psize);
        cout << "\r                           \r";
        cout << "Number of FEC groups : " << _fecGroupList.size() << flush;
      }
    } // 

    while(psize < SimLimit && !_fC.stop()){
      SVgenerator_1();
      //cerr << "psize = " << psize << endl;
      SimulateAll();
      psize += 64;
      //cerr << "Up: " << endl;
      updateFECgroups();
      if(psize % 4096 == 0)_fC.add(_fecGroupList.size());
      reportSimLog(64);
      cout << "\r                           \r";
      cout << "Number of FEC groups : " << _fecGroupList.size() << flush;
    }
  }
  cout << endl;
  // cout << "Number of FEC groups : " << _fecGroupList.size() << endl;
  cout << psize << " patterns simulated." << endl;
  sortFECList();
  _nowPack = 0;
  clearNowPackList();
}

void
CirMgr::fileSim(ifstream& patternFile)
{
  // Simulate from a pattern file
  string inS;
  size_t psize = 0; // number of patterns
  // A & 63 == A % 64
  if(_fecGroupList.empty()){
    buildFECgroupList();
  }
  /*if(_fecGroupList.empty()){
    cout << psize << " patterns simulated." << endl;
    return;
  }*/
  // printFECPairs();
  // cout << endl;

  if(_nowPack == 0){
    _nowPack = new SimValuePack;
    _nowPack->clear();
    _nowPack->resize(_I, SimValue(0)); // ...
    resetNowPack();
  }

  /*cout << "Size of size_t : " << sizeof(size_t) << endl;
  cout << (size_t)((size_t)1 << 63) << endl;
  for(size_t i = 0; i < _I; ++i) {
    cout << "(" << i << ") ";
    (*_nowPack)[i].printVal();
    cout << endl;
  }*/

  while(patternFile >> inS){
    if(inS.length() != _I){
      cout << "Error: Pattern(" << inS << ") length(" << inS.length();
      cout << ") does not match the number of inputs(" << _I << ") in a circuit!!" << endl;
      resetNowPack();
      return;
    }

    
    for(size_t i = 0; i < _I; ++i){
      if((inS[i] != '0') && (inS[i] != '1')){
        cout << "Error: Pattern(" << inS << ") contains a non-0/1 character(\'" << inS[i] << "\')." << endl;
        resetNowPack();
        return;
      }

      (*_nowPack)[i].modify(psize & 63, (inS[i] == '0') ? false : true);
    }
    ++psize;
    //cerr << "psize : " << psize << endl;

    if((psize & 63) == 0){
      // It's simulate time !
      SimulateAll();
      updateFECgroups();
      
      resetNowPack();
      reportSimLog(64);
    }
  }

  /*for(size_t i = 0; i < _I; ++i) {
    cout << "(" << i << ") ";
    (*_nowPack)[i].printVal();
    cout << endl;
  }*/

  if((psize & 63) != 0){
    // If there are still some patterns have not yet been simulated
    SimulateAll();
    updateFECgroups();

    resetNowPack();
    reportSimLog(psize & 63);
  }

  cout << "Number of FEC groups : " << _fecGroupList.size() << endl;
  cout << psize << " patterns simulated." << endl;
  sortFECList();
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/

void 
CirMgr::buildFECgroupList(){
  // Only AIG and CONST0 gates will be included in the FEC pairs
  FECgroup* newGroup = new FECgroup;
  newGroup->clear();
  _fecGroupList.clear();
  
  for(size_t i = 0; i < _netList.size(); ++i){
    if(_netList[i]->isAig()){
      newGroup->insert(_netList[i]);
    }
  }
  
  newGroup->insert(_totalList[0]);
  if(newGroup->size() == 1){
    delete newGroup;
    return;
  }
  _fecGroupList.push_back(newGroup);
}

size_t 
CirMgr::FECgroupSize() const {
  return _fecGroupList.size();
}

void 
CirMgr::SimulateAll() {
  // Using _nowPack to simulate all gates
  assert(_nowPack->size() == _I);

  for(size_t i = 0; i < _I; ++i){
    _PIList[i]->modifySimValue((*_nowPack)[i]);
  }

  for(size_t i = 0; i < _netList.size(); ++i){
    if((_netList[i]->getTypeStr() == "AIG") || (_netList[i]->getTypeStr() == "PO")){
      _netList[i]->Simulate();
    }
  }
}

void 
CirMgr::SimulateList(const size_t _ith){
  // Using _nowPackList[_ith] to simulate all gates

  assert(_ith >=0 && _ith < _nowPackList.size());
  assert(_nowPackList[_ith]->size() == _I);

  for(size_t i = 0; i < _I; ++i){
    _PIList[i]->modifySimValue((*_nowPackList[_ith])[i]);
  }

  for(size_t i = 0; i < _netList.size(); ++i){
    if((_netList[i]->getTypeStr() == "AIG") || (_netList[i]->getTypeStr() == "PO")){
      _netList[i]->Simulate();
    }
  }
}

void 
CirMgr::updateFECgroups(){
  if(_fecGroupList.empty()) return;
  int oSize = (int)_fecGroupList.size(); // original size of the FEC group list

  for(int i = 0; i < oSize; ++i){
    if((_fecGroupList[i])->size() == 1) continue;
    // For each group in fecGroupList
    SMap newFECgroups; // < SimValue, FECgroup* >
    newFECgroups.clear();

    FECgroup* nowGroup = _fecGroupList[i]; // set< CirGate* >
    FECgroup::iterator it = (*nowGroup).begin();

    for( ; it != (*nowGroup).end(); ++it){
      // For each gate in nowGroup
      SMap::iterator it2 = newFECgroups.find((*it)->getSimValue());
      if(it2 != newFECgroups.end()){
        // Same SimValue as the gates in it2
        (it2->second)->insert(*it);
      } else {
        FECgroup* newGroup = new FECgroup;
        newGroup->clear();
        newGroup->insert(*it);
        newFECgroups.insert(make_pair((*it)->getSimValue(), newGroup));
      }
    }

    // Collect all the FEC groups in newFECgroups
    SMap::iterator it3 = newFECgroups.begin();
    _fecGroupList[i]->clear();
    delete _fecGroupList[i];
    _fecGroupList[i] = 0;

    if((it3->second)->size() > 1){
      _fecGroupList[i] = (it3->second);
    } else {
      it3->second->clear();
      delete it3->second;
      if(i == oSize - 1){
        if(oSize == (int)_fecGroupList.size()){
          _fecGroupList.pop_back();
        } else {
          _fecGroupList[i] = _fecGroupList.back();
          _fecGroupList.pop_back();
        }
      } else {
        if(oSize == (int)_fecGroupList.size()){
          _fecGroupList[i] = _fecGroupList.back();
          _fecGroupList.pop_back();
          --oSize;
          --i;
        } else {
          _fecGroupList[i] = _fecGroupList.back();
          _fecGroupList.pop_back();
        }
      }
    }
    ++it3;

    for( ; it3 != newFECgroups.end(); ++it3){
      if((it3->second)->size() > 1)
        _fecGroupList.push_back(it3->second);
      else{
        it3->second->clear();
        delete it3->second;
      }
    }
  }
}

void 
CirMgr::resetNowPack(){
  if(_nowPack == 0) return;
  assert(_nowPack->size() == _I);

  for(size_t i = 0; i < _I; ++i){
    (*_nowPack)[i].reset();
  }
  // Reset all SimValues to zero
}

void 
CirMgr::reportSimLog(const size_t patternsN) const {
  if(_simLog == 0) return;
  
  for(size_t i = 0; i < patternsN; ++i){
    // report PIs
    for(size_t j = 0; j < _I; ++j){
      *_simLog << ((_PIList[j]->getSimBit(i)) ? '1' : '0');
    }
    *_simLog << ' ';
    // report POs
    for(size_t j = 0; j < _O; ++j){
      *_simLog << ((_POList[j]->getSimBit(i)) ? '1' : '0');
    }
    *_simLog << endl;
  }
}

void 
CirMgr::SVgenerator_all(){
  // Generates all permutaions of SimValue
  assert(_I <= ILimit);
  assert(_nowPackList.empty());

  // Decide how many packs
  size_t numPack = (_I <= 6) ? ONE : (ONE << (_I - 6));
  for(size_t i = 0; i < numPack; ++i){
    SimValuePack* newPack = new SimValuePack;
    _nowPackList.push_back(newPack);
    _nowPackList[i]->clear();
  }

  for(size_t i = 1; i <= _I; ++i){
    size_t val = 0;
    if(i <= 6){
      size_t _period = ONE << i;
      size_t _base = ((ONE << (_period >> ONE)) - ONE) << (_period >> ONE);

      for(size_t j = 0; j < 64; j += _period){
        val = (val | (_base << j));
      }
      SimValue tmpSV(val);
      for(size_t j = 0; j < numPack; ++j){
        _nowPackList[j]->push_back(tmpSV);
      }
    } else {
      size_t _period = ONE << (i - 5);
      SimValue SV0(0);
      SimValue SVI = ~SV0;
      for(size_t j = 0; j < numPack; j += _period){
        for(size_t k = 0; k < (_period >> 1); ++k){
          _nowPackList[j + k]->push_back(SV0);
        }
        for(size_t k = _period >> 1; k < _period; ++k){
          _nowPackList[j + k]->push_back(SimValue(SVI));
        }
      }
    }
  }

  // Check 
  /*for(size_t i = 0; i < _I; ++i){
    (*_nowPackList[0])[i].printVal();
    cout << endl;
  }*/
}

void 
CirMgr::SVgenerator_1(){
  // Generates a pack of SimValue at a time. 
  // Using random mask
  size_t _base = (size_t)rG(rLIM) | (((size_t)rG(rLIM)) << 32);

  if(_nowPack == 0){
    _nowPack = new SimValuePack;
    _nowPack->clear();
    _nowPack->resize(_I, SimValue(0)); // ...
    resetNowPack();
  }
  
  (*_nowPack)[0] = SimValue(_base);
  for(size_t i = 1; i < _I; ++i){
    size_t _Mask = (ONE << (i & 63));
    int jj = 14;
    while(jj > 0){
      size_t ri = rG(64);
      if(_Mask & (ONE << ri)) continue;
      else {
        _Mask |= (ONE << ri);
        --jj;
      }
    }
    (*_nowPack)[i] = SimValue(_base ^ _Mask);

    //(*_nowPack)[i].printVal(); cerr << endl;
  }
}

void 
CirMgr::sortFECList(){
  // Sort the whole FEC list
  if(_fecGroupList.size() <= 1) return;
  tmpList.clear();
  tmpList.resize(_fecGroupList.size(), 0);

  MergeSort(0, _fecGroupList.size() - 1);
}

void 
CirMgr::MergeSort(size_t _l, size_t _r){
  if(_l >= _r) return;

  size_t _m = (_l + _r) >> 1;
  MergeSort(_l, _m);
  MergeSort(_m + 1, _r);
  Merge(_l, _m, _r);
}

void 
CirMgr::Merge(size_t _l, size_t _m, size_t _r){
  size_t i = _l, j = _m + 1, k = _l;
  while(i <= _m && j <= _r){
    if(FECgroupCmp(_fecGroupList[i], _fecGroupList[j])){
      tmpList[k++] = _fecGroupList[i++];
    } else {
      tmpList[k++] = _fecGroupList[j++];
    }
  }
  while(i <= _m) tmpList[k++] = _fecGroupList[i++];
  while(j <= _r) tmpList[k++] = _fecGroupList[j++];
  for(i = _l; i <= _r; ++i) _fecGroupList[i] = tmpList[i];

}


void 
CirMgr::clearNowPackList(){
  if(_nowPackList.empty()) return;

  for(size_t i = 0; i < _nowPackList.size(); ++i){
    if(_nowPackList[i] != 0)
      delete _nowPackList[i];
  }
  _nowPackList.clear();
  _packs = 0;
}

void 
CirMgr::recycleNowPackList(){
  if(_nowPackList.empty()) return;

  for(size_t i = 0; i < _nowPackList.size(); ++i){
    if(_nowPackList[i] != 0)
      _goodPackList.push_back(_nowPackList[i]); // Recycle packs to goodPackList
  }
  _nowPackList.clear();
  _packs = 0;
}