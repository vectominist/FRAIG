/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;

/**************************************/
/*   class CirGate member functions   */
/**************************************/
void
CirGate::reportGate() const
{
   cout << "Gate " << _gateIndex;
}

void
CirGate::reportFanin(int level) const
{
   assert (level >= 0);
   
   const unsigned nowFlag = getFlag();
   cout << getTypeStr() << " " << getGateIndex() << endl;
   newFlag(); // update flag

   set<const CirGate*> traversedList;
   traversedList.insert(this);

   for(size_t i = 0; i < getFaninSize(); ++i){
      faninDFS(getFaninList(i), 1, level, nowFlag, getInvertedList(i), traversedList);
   }

   prevFlag();
   for(size_t i = 0; i < getFaninSize(); ++i){
      faninClean(getFaninList(i), 1, level, nowFlag);
   }
}

void
CirGate::reportFanout(int level) const
{
   assert (level >= 0);
   const unsigned nowFlag = getFlag();
   cout << getTypeStr() << " " << getGateIndex() << endl;
   newFlag(); // update flag

   set<const CirGate*> traversedList;
   traversedList.insert(this);

   for(size_t i = 0; i < getFanoutSize(); ++i){
      fanoutDFS(getFanoutList(i), 1, level, nowFlag, getFanoutList(i)->getInvertedListG(this), traversedList);
   }

   prevFlag();
   for(size_t i = 0; i < getFanoutSize(); ++i){
      fanoutClean(getFanoutList(i), 1, level, nowFlag);
   }
}

void CirGate::reportSimValue() const{
   _sv.printVal();
}

/**************************************/
/*  class CirGate private functions   */
/**************************************/

void CirGate::printSpace(const int _depth) const {
   for(int i = 0; i < _depth; ++i)
      cout << "  ";
}

void
CirGate::faninDFS
   (CirGate* nowGate, const int _depth, const int _level, const unsigned _nowFlag, 
      const bool _inv, set<const CirGate*>& _tList) const {
   if(nowGate == 0) return;

   printSpace(_depth);
   if(_inv) cout << '!';
   cout << nowGate->getTypeStr() << " " << nowGate->getGateIndex();
   if(nowGate->getFlag() == _nowFlag) nowGate->newFlag();
   if(_depth == _level) { cout << endl; return; }
   if(_tList.count(nowGate) > 0){
      cout << " (*)" << endl;
      return;
   }
   cout << endl;

   if(nowGate->getFaninSize() > 0) _tList.insert(nowGate);

   for(size_t i = 0; i < nowGate->getFaninSize(); ++i){
      faninDFS(nowGate->getFaninList(i), _depth + 1, _level, _nowFlag, 
         nowGate->getInvertedList(i), _tList);
   }
}
void
CirGate::fanoutDFS
   (CirGate* nowGate, const int _depth, const int _level, const unsigned _nowFlag, 
      const bool _inv, set<const CirGate*>& _tList) const {
   if(nowGate == 0) return;

   printSpace(_depth);
   if(_inv) cout << '!';
   cout << nowGate->getTypeStr() << " " << nowGate->getGateIndex();
   if(nowGate->getFlag() == _nowFlag) nowGate->newFlag();
   if(_depth == _level) { cout << endl; return; }
   if(_tList.count(nowGate) > 0){
      cout << " (*)" << endl;
      return;
   }
   cout << endl;

   if(nowGate->getFanoutSize() > 0) _tList.insert(nowGate);

   for(size_t i = 0; i < nowGate->getFanoutSize(); ++i){
      fanoutDFS(nowGate->getFanoutList(i), _depth + 1, _level, _nowFlag, 
         nowGate->getFanoutList(i)->getInvertedListG(nowGate), _tList);
   }
}

void
CirGate::fanoutClean
   (CirGate* nowGate, const int _depth, const int _level, const unsigned _nowFlag) const {
   if(nowGate == 0) return;
   if(nowGate->getFlag() == _nowFlag) return;

   nowGate->prevFlag();

   if(_depth == _level) return;

   for(size_t i = 0; i < nowGate->getFanoutSize(); ++i){
      fanoutClean(nowGate->getFanoutList(i), _depth + 1, _level, _nowFlag);
   }
}

void
CirGate::faninClean
   (CirGate* nowGate, const int _depth, const int _level, const unsigned _nowFlag) const {
   if(nowGate == 0) return;
   if(nowGate->getFlag() == _nowFlag) return;

   nowGate->prevFlag();

   if(_depth == _level) return;

   for(size_t i = 0; i < nowGate->getFaninSize(); ++i){
      faninClean(nowGate->getFaninList(i), _depth + 1, _level, _nowFlag);
   }
}

/**************************************/
/*  class CirGatePI member functions  */
/**************************************/
void CirGatePI::printGate() const{
   cout << "PI  " << _gateIndex;
   if(_name != 0) cout << " (" << _name << ")";
}
void
CirGatePI::reportGate() const
{
   cout << "================================================================================" << endl;
   string _output = "= PI(" + to_string(_gateIndex) + ")";
   if(_name != 0) _output += "\"" + getName() + "\"";
   _output += ", line " + to_string(_lineNo);

   // cout << setw(49) << left << _output << "=" << endl;
   cout << _output << endl;

   cout << "= FECs:";  cirMgr->printFEC(_gateIndex); cout << endl;
   cout << "= Value: "; _sv.printVal();               cout << endl;
   cout << "================================================================================" << endl;
}
void
CirGatePI::reportFanin(int level) const
{
   assert (level >= 0);
   cout << getTypeStr() << " " << getGateIndex() << endl;
}
/*
void
CirGatePI::reportFanout(int level) const
{
   assert (level >= 0);
}*/


/**************************************/
/*  class CirGatePO member functions  */
/**************************************/
void CirGatePO::printGate() const{
   cout << "PO  " << _gateIndex << " ";
   if(_fanin->getTypeStr() == "UNDEF") cout << '*';
   if(_inverted) cout << '!';
   cout << _fanin->getGateIndex();
   if(_name != 0) cout << " (" << _name << ")";
}
void
CirGatePO::reportGate() const
{
   cout << "================================================================================" << endl;
   string _output = "= PO(" + to_string(_gateIndex) + ")";
   if(_name != 0) _output += "\"" + getName() + "\"";
   _output += ", line " + to_string(_lineNo);

   //cout << setw(49) << left << _output << "=" << endl;

   cout << _output << endl;

   cout << "= FECs:";  cirMgr->printFEC(_gateIndex); cout << endl;
   cout << "= Value: "; _sv.printVal();               cout << endl;
   cout << "================================================================================" << endl;
}
/*void
CirGatePO::reportFanin(int level) const
{
   assert (level >= 0);
}*/

void
CirGatePO::reportFanout(int level) const
{
   assert (level >= 0);
   cout << getTypeStr() << " " << getGateIndex() << endl;
}

/**************************************/
/* class CirGateAIG member functions  */
/**************************************/
void CirGateAIG::printGate() const{
   cout << "AIG " << _gateIndex << " ";

   if(_faninList[0]->getTypeStr() == "UNDEF") cout << '*';
   if(_inverted[0]) cout << '!';
   cout << _faninList[0]->getGateIndex() << " ";
   if(_faninList[1]->getTypeStr() == "UNDEF") cout << '*';
   if(_inverted[1]) cout << '!';
   cout << _faninList[1]->getGateIndex();
}
void
CirGateAIG::reportGate() const
{
   cout << "================================================================================" << endl;
   string _output = "= AIG(" + to_string(_gateIndex) + ")";
   _output += ", line " + to_string(_lineNo);

   // cout << setw(49) << left << _output << "=" << endl;
   cout << _output << endl;

   cout << "= FECs:";  cirMgr->printFEC(_gateIndex); cout << endl;
   cout << "= Value: "; _sv.printVal();               cout << endl;
   cout << "================================================================================" << endl;
}
/*void
CirGateAIG::reportFanin(int level) const
{
   assert (level >= 0);
}

void
CirGateAIG::reportFanout(int level) const
{
   assert (level >= 0);
}*/


/**************************************/
/* class CirGateCONST member functions*/
/**************************************/
void CirGateCONST::printGate() const{
   cout << "CONST0";
}
void
CirGateCONST::reportGate() const
{
   cout << "================================================================================" << endl;
   string _output = "= CONST(" + to_string(_gateIndex) + ")";
   _output += ", line " + to_string(_lineNo);

   // cout << setw(49) << left << _output << "=" << endl;
   cout << _output << endl;

   cout << "= FECs:";  cirMgr->printFEC(_gateIndex); cout << endl;
   cout << "= Value: "; _sv.printVal();               cout << endl;
   cout << "================================================================================" << endl;
}
void
CirGateCONST::reportFanin(int level) const
{
   assert (level >= 0);
   cout << getTypeStr() << " " << getGateIndex() << endl;
}

/*void
CirGateCONST::reportFanout(int level) const
{
   assert (level >= 0);
}*/


/**************************************/
/* class CirGateUNDEF member functions*/
/**************************************/
void CirGateUNDEF::printGate() const{
   
}
void
CirGateUNDEF::reportGate() const
{
   cout << "================================================================================" << endl;
   string _output = "= UNDEF(" + to_string(_gateIndex) + ")";
   _output += ", line " + to_string(_lineNo);

   // cout << setw(49) << left << _output << "=" << endl;
   cout << _output << endl;

   cout << "= FECs:";  cirMgr->printFEC(_gateIndex); cout << endl;
   cout << "= Value: "; _sv.printVal();               cout << endl;
   cout << "================================================================================" << endl;
}
void
CirGateUNDEF::reportFanin(int level) const
{
   assert (level >= 0);
   cout << getTypeStr() << " " << getGateIndex() << endl;
}

/*void
CirGateUNDEF::reportFanout(int level) const
{
   assert (level >= 0);
}*/

