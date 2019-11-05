/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
  GateList tmpList(_floatList2.size(), 0);

  GateSet::iterator it = _floatList2.begin();
  for(size_t i = 0 ; it != _floatList2.end(); ++it, ++i){
    tmpList[i] = *it;
  }
  _floatList2.clear();

  for(size_t i = 0; i < tmpList.size(); ++i){
    if(tmpList[i] == 0) continue;
    if(tmpList[i]->getTypeStr() == "CONST") continue;
    if(tmpList[i]->getTypeStr() != "PI"){
      size_t sz = tmpList[i]->getFaninSize();

      // remove from fanins' fanout
      
      for(size_t j = 0; j < sz; ++j){
        CirGate* _g = tmpList[i]->getFaninList(j);
        _g->removeFanoutGate(tmpList[i]);
        if((_g->getFanoutSize() == 0)/* && (_g->getTypeStr() == "PI")*/){
          // no fanout
          tmpList.push_back(_g);
        }
      }
      
      // remove from _floatList1
      /*for(size_t j = 0; j < _floatList1.size(); ++j){
        if(_floatList1[j] == (*it)){
          _floatList1.erase(_floatList1.begin() + j);
          break;
        }
      }*/

      cout << "Sweeping: " << tmpList[i]->getTypeStr()
          << "(" << tmpList[i]->getGateIndex() << ") removed..." << endl;
      
      deleteGate(tmpList[i]);
      tmpList[i] = 0;
      // _floatList2[i] = 0;
    }
  }


  /*size_t i = 0;
  while(i < _floatList2.size()){
    if(_floatList2[i] == 0){
      _floatList2.erase(_floatList2.begin() + i);
    } else {
      ++i;
    }
  }*/
  updateFloating();
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
  for(size_t i = 0; i < _netList.size(); ++i){
    if(_netList[i] == 0) continue;
    //cout << "OPT " << i << endl;
    if(_netList[i]->getTypeStr() == "AIG"){
      CirGate* _G = _netList[i];
      CirGate* _Gin[2];
      bool _Gv[2];
      
      //cerr << "Fanin size: " << _G->getFaninSize() << endl;
      _Gin[0] = _G->getFaninList(0);   _Gin[1] = _G->getFaninList(1);
      _Gv[0] = _G->getInvertedList(0); _Gv[1] = _G->getInvertedList(1);
      /*cerr << "OK ?" << endl;
      cerr << _Gin[0] << " " << _Gin[1] << endl;
      cerr << _Gin[0]->getGateIndex() << endl;
      cerr << _Gin[0]->getTypeStr() << endl;
      cerr << _Gv[0] << " " << _Gv[1] << endl;*/

      if(_Gin[0]->getTypeStr() == "CONST"){
        if(_Gv[0] == true){ // 1 -> _Gin[1]
          replaceGate(_G, _Gin[1], _Gv[1]);
          optMessage(_Gin[1]->getGateIndex(), _Gv[1], _G->getGateIndex());
        } else { // 0
          replaceGate(_G, _Gin[0], false);
          optMessage(0, false, _G->getGateIndex());
        }
      }
      else if(_Gin[1]->getTypeStr() == "CONST"){
        if(_Gv[1] == true){ // 1 -> _Gin[0]
          replaceGate(_G, _Gin[0], _Gv[0]);
          optMessage(_Gin[0]->getGateIndex(), _Gv[0], _G->getGateIndex());
        } else { // 0
          replaceGate(_G, _Gin[1], false);
          optMessage(0, false, _G->getGateIndex());
        }
      }
      else if(_Gin[0] == _Gin[1]){
        //cerr << "Cmp" << endl;
        if((!_Gv[0] && !_Gv[1]) || (_Gv[0] && _Gv[1])){ // (a & a) or (!a & !a) -> a or !a
          replaceGate(_G, _Gin[0], _Gv[0]);
          optMessage(_Gin[0]->getGateIndex(), _Gv[0], _G->getGateIndex());
        } else { // CONST
          replaceGate(_G, _totalList[0], false);
          optMessage(0, false, _G->getGateIndex());
        }
      } else {
        continue;
      }

      // delete
      deleteGate(_G);
    }
  }
  buildNetlist();
  updateFloating();
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/

void CirMgr::replaceGate(CirGate* nowGate, CirGate* newGate, bool _inv){
  assert(nowGate != 0);
  assert(newGate != 0);
  //cerr << "Replace" << endl;
  for(size_t i = 0; i < nowGate->getFaninSize(); ++i){
    assert(nowGate->getFaninList(i)->removeFanoutGate(nowGate));
  }

  for(size_t i = 0; i < nowGate->getFanoutSize(); ++i){
    //cerr << "replace fanout : " << i << endl;
    newGate->addFanout(nowGate->getFanoutList(i));

    for(size_t j = 0; j < nowGate->getFanoutList(i)->getFaninSize(); ++j){
      if(nowGate->getFanoutList(i)->getFaninList(j) == nowGate){
        nowGate->getFanoutList(i)->modifyFanin(newGate, j);

        if(_inv == true){
          if(nowGate->getFanoutList(i)->getInvertedList(j) == true)
            nowGate->getFanoutList(i)->modifyInvert(false, j);
          else 
            nowGate->getFanoutList(i)->modifyInvert(true, j);
        }
      }
    }

  }
  
}

void CirMgr::deleteGate(CirGate* nowGate){
  unsigned _gateID = nowGate->getGateIndex();
  /*for(size_t i = 0; i < nowGate->getFanoutSize(); ++i){
  }*/
  for(size_t i = 0; i < nowGate->getFaninSize(); ++i){
    nowGate->getFaninList(i)->removeFanoutGate(nowGate);
  }

  if(nowGate->isAig()) --_A;
  delete nowGate;
  _totalList[_gateID] = 0;
}

void CirMgr::optMessage(unsigned newID, bool newInv, unsigned oldID) const{
  cout << "Simplifying: " << newID << " merging ";
  if(newInv == true) cout << "!";
  cout << oldID << "..." << endl;
}

