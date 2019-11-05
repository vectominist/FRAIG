/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <cstring>
#include <vector>
#include <iostream>
#include <set>
#include <algorithm>
#include "cirDef.h"
#include "sat.h"
#include "myHashMap.h"

using namespace std;

class CirGate;

// bool GateCmp(const CirGate* _g1, const CirGate* _g2);

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
// TODO: Define your own data members and member functions, or classes
class CirGate
{
  // friend CirMgr;
public:
  CirGate(): _gateIndex(0), _lineNo(0), _flag(0), _topOrder(0) 
    { _flag = new unsigned; *(_flag) = 0; _sv = SimValue(0); }
  CirGate(const unsigned _gindex): _gateIndex(_gindex), _lineNo(0), _flag(0), _topOrder(0)
    { _flag = new unsigned; *(_flag) = 0; _sv = SimValue(0); }
  CirGate(const unsigned _gindex, const unsigned _lNo):
    _gateIndex(_gindex), _lineNo(_lNo), _flag(0), _topOrder(0)
    { _flag = new unsigned; *(_flag) = 0; _sv = SimValue(0); }
  virtual ~CirGate() { delete _flag; _flag = 0; }

  // Basic access methods
  virtual string getTypeStr() const { return "CirGate"; }
  unsigned getLineNo() const { return _lineNo; }
  unsigned getGateIndex() const { return _gateIndex; }
  virtual bool isAig() const { return false; }

  // Helper functions
  virtual void modifyName(const string& _str) {}
  virtual string getName() const { return ""; }
  virtual void addFanout(CirGate* _g) {}
  virtual void addFanin(CirGate* _g) {}
  virtual void modifyFanin(CirGate* _g, size_t _ith) {}
  virtual void addInverted(const bool _inv) {}
  virtual void modifyInvert(const bool _inv, size_t _ith) {}

  unsigned getFlag() const { return *_flag; }
  void newFlag() const { (*_flag)++; }
  void prevFlag() const { (*_flag)--; }

  virtual size_t getFaninSize() const { return 0; }
  virtual CirGate* getFaninList(size_t _ith) const { return 0; }
  virtual bool getInvertedList(size_t _ith) const { return false; }
  virtual bool getInvertedListG(const CirGate* _g) const { return false; }

  virtual size_t getFanoutSize() const { return 0; }
  virtual CirGate* getFanoutList(size_t _ith) const { return 0; }
  virtual bool removeFanoutGate(const CirGate* _g) { return false; }
  // virtual void sortFanout() {}

  // For simulation
  SimValue getSimValue() const { return _sv; }
  bool getSimBit(const size_t _ith) const { return _sv.getBit(_ith); }
  void modifySimValue(const SimValue _newSV) { _sv = _newSV; }
  virtual void Simulate() {}

  int getTopOrder() const { return _topOrder; }
  virtual void modifyTopOrder(const int _t) { _topOrder = _t; }

  // Printing functions
  virtual void printGate() const = 0;
  virtual void reportGate() const;
  virtual void reportFanin(int level) const;
  virtual void reportFanout(int level) const;
  void reportSimValue() const;

private:

protected:
  // GateList _faninList;              // Fan in list
  // GateList _fanoutList;             // Fan out list
  // vector<bool> _inverted;           // Inverted or not of _faninList (0:no, 1:inverted)
  unsigned _gateIndex;                 // Index of gate
  // string _typeStr;                  // Type of gate
  unsigned _lineNo;                    // Line number (appeared in the AIGER file)
  // string _name;                     // Name of the gate
  unsigned* _flag;                     // Flag for DFS
  SimValue  _sv;                       // Simulation value of gate
  int  _topOrder;                      // Topological order of gate
  void faninDFS(CirGate*, const int, const int, const unsigned, const bool, set<const CirGate*>&) const;
  void fanoutDFS(CirGate*, const int, const int, const unsigned, const bool, set<const CirGate*>&) const;
  void faninClean(CirGate*, const int, const int, const unsigned) const;
  void fanoutClean(CirGate*, const int, const int, const unsigned) const;
  void printSpace(const int) const;
};

/*bool GateCmp(const CirGate* _g1, const CirGate* _g2) {
   return _g1->getGateIndex() < _g2->getGateIndex();
}*/

struct GateCmp{
  bool operator () (const CirGate* _g1, const CirGate* _g2) const {
    return (_g1->getGateIndex()) < (_g2->getGateIndex());
  }
};

class CirGatePI : public CirGate {
public:
  //CirGatePI() {}
  CirGatePI(const unsigned _gindex, const unsigned _lNo) : 
    CirGate(_gindex, _lNo), _name(0)
  {
    _fanoutList.clear();
  }
  ~CirGatePI() { _fanoutList.clear(); if(_name) delete _name; }

  virtual string getTypeStr() const { return "PI"; }

  // Helper functions
  virtual void modifyName(const string& _str) {
    _name = new char[_str.length() + 1];
    strcpy(_name, _str.c_str());
  }
  virtual string getName() const { return (_name) ? string(_name) : ""; }
  virtual void addFanout(CirGate* _g)  { _fanoutList.push_back(_g); }
  
  virtual size_t getFanoutSize() const { return _fanoutList.size(); }
  virtual CirGate* getFanoutList(size_t _ith) const {
    assert(_ith < _fanoutList.size()); return _fanoutList[_ith];
  }
  virtual bool removeFanoutGate(const CirGate* _g) {
    bool _exist = false;
    size_t i = 0;
    while(i < _fanoutList.size()){
      if(_fanoutList[i] == _g){ _fanoutList.erase(_fanoutList.begin() + i); _exist = true; break; }
      else ++i;
    } return _exist;
  }
  //virtual void sortFanout() { sort(_fanoutList.begin(), _fanoutList.end(), GateCmp); }

  // Printing functions
  virtual void printGate() const;
  virtual void reportGate() const;
  virtual void reportFanin(int level) const;
  // virtual void reportFanout(int level) const;

private:
  GateList _fanoutList;             // Fan out list
  char* _name;                      // Name of the gate
};

class CirGatePO : public CirGate {
public:
  // CirGatePO() {}
  CirGatePO(const unsigned _gindex, const unsigned _lNo):
    CirGate(_gindex, _lNo), _name(0)
  {
    _fanin = 0;
    _inverted = 0;
  }
  ~CirGatePO() { if(_name) delete _name; }

  virtual string getTypeStr() const { return "PO"; }

  // Helper functions
  virtual void modifyName(const string& _str) {
    _name = new char[_str.length() + 1];
    strcpy(_name, _str.c_str());
  }
  virtual string getName() const { return (_name) ? string(_name) : ""; }
  virtual void addFanin(CirGate* _g) { _fanin = _g; }
  virtual void modifyFanin(CirGate* _g, size_t _ith) { _fanin = _g; }
  virtual void addInverted(const bool _inv) { _inverted =_inv; }
  virtual void modifyInvert(const bool _inv, size_t _ith) { assert(_ith < 1); _inverted = _inv; }

  virtual size_t getFaninSize() const { return 1; }
  virtual CirGate* getFaninList(size_t _ith) const {
    assert(_ith == 0); return _fanin;
  }
  virtual bool getInvertedList(size_t _ith) const {
    assert(_ith == 0); return _inverted;
  }
  virtual bool getInvertedListG(const CirGate* _g) const {
    assert(_g != 0); return _inverted;
  }

  virtual void Simulate() {
    _sv = (_inverted) ? ~(_fanin->getSimValue()) : (_fanin->getSimValue());
  }

  // Printing functions
  virtual void printGate() const;
  virtual void reportGate() const;
  // virtual void reportFanin(int level) const;
  virtual void reportFanout(int level) const;

private:
  CirGate* _fanin;                  // Fan in list
  bool _inverted;                   // Inverted or not of _faninList (0:no, 1:inverted)
  char* _name;                      // Name of the gate
};

class CirGateAIG : public CirGate {
public:
  // CirGateAIG() {}
  CirGateAIG(const unsigned _gindex, const unsigned _lNo):
    CirGate(_gindex, _lNo)
  {
    _faninList[0] = _faninList[1] = 0;
    _inverted[0] = _inverted[1] = 0;
    _fanoutList.clear();
  }
  ~CirGateAIG() { _fanoutList.clear(); }

  virtual string getTypeStr() const { return "AIG"; }
  virtual bool isAig() const { return true; }

  // Helper functions
  virtual void addFanout(CirGate* _g) { _fanoutList.push_back(_g); }
  virtual void addFanin(CirGate* _g) { _faninList[_faninList[0] ? 1 : 0] = _g; }
  virtual void modifyFanin(CirGate* _g, size_t _ith) { _faninList[_ith] = _g; }
  virtual void addInverted(const bool _inv) { _inverted[_faninList[0] ? 1 : 0] =_inv; }
  virtual void modifyInvert(const bool _inv, size_t _ith) { assert(_ith < 2); _inverted[_ith] = _inv; }

  virtual size_t getFaninSize() const { return 2; }
  virtual CirGate* getFaninList(size_t _ith) const { 
    assert(_ith < 2); return _faninList[_ith];
  }
  virtual bool getInvertedList(size_t _ith) const {
    assert(_ith < 2); return _inverted[_ith];
  }
  virtual bool getInvertedListG(const CirGate* _g) const {
    assert(_g != 0);
    if(_faninList[0] == _g) return _inverted[0];
    else if(_faninList[1] == _g) return _inverted[1];
    else { cerr << "Not Found" << endl; return false; }
  }
  
  virtual size_t getFanoutSize() const { return _fanoutList.size(); }
  virtual CirGate* getFanoutList(size_t _ith) const {
    assert(_ith < _fanoutList.size()); return _fanoutList[_ith];
  }
  virtual bool removeFanoutGate(const CirGate* _g) {
    bool _exist = false;
    size_t i = 0;
    while(i < _fanoutList.size()){
      if(_fanoutList[i] == _g){ _fanoutList.erase(_fanoutList.begin() + i); _exist = true; break; }
      else ++i;
    } return _exist;
  }
  // virtual void sortFanout() { sort(_fanoutList.begin(), _fanoutList.end(), GateCmp); }

  virtual void Simulate() {
    _sv = ((_inverted[0]) ? ~(_faninList[0]->getSimValue()) : (_faninList[0]->getSimValue()))
           & ((_inverted[1]) ? ~(_faninList[1]->getSimValue()) : (_faninList[1]->getSimValue()));
  }

  // Printing functions
  virtual void printGate() const;
  virtual void reportGate() const;
  // virtual void reportFanin(int level) const;
  // virtual void reportFanout(int level) const;

private:
  CirGate* _faninList[2];           // Fan in list
  GateList _fanoutList;             // Fan out list
  bool _inverted[2];                // Inverted or not of _faninList (0:no, 1:inverted)
};

class CirGateCONST : public CirGate {
public:
  CirGateCONST(): CirGate(){
    /*_lineNo = 0;
    _gateIndex = 0;*/
    _topOrder = -1; // Only CONST0 has a topOrder of -1
  }
  ~CirGateCONST() { _fanoutList.clear(); }

  virtual string getTypeStr() const { return "CONST"; }

  // Helper functions
  virtual void addFanout(CirGate* _g) { _fanoutList.push_back(_g); }

  virtual size_t getFanoutSize() const { return _fanoutList.size(); }
  virtual CirGate* getFanoutList(size_t _ith) const {
    assert(_ith < _fanoutList.size()); return _fanoutList[_ith];
  }
  virtual bool removeFanoutGate(const CirGate* _g) {
    bool _exist = false;
    size_t i = 0;
    while(i < _fanoutList.size()){
      if(_fanoutList[i] == _g){ _fanoutList.erase(_fanoutList.begin() + i); _exist = true; break; }
      else ++i;
    } return _exist;
  }
  // virtual void sortFanout() { sort(_fanoutList.begin(), _fanoutList.end(), GateCmp); }
  virtual void modifyTopOrder(const int _t) { return; }

  // Printing functions
  virtual void printGate() const;
  virtual void reportGate() const;
  virtual void reportFanin(int level) const;
  // virtual void reportFanout(int level) const;

private:
  GateList _fanoutList;             // Fan out list
};

class CirGateUNDEF : public CirGate {
public:
  // CirGateUNDEF() {}
  CirGateUNDEF(const unsigned _gindex): CirGate(_gindex) {
  }
  ~CirGateUNDEF() { _fanoutList.clear(); }

  virtual string getTypeStr() const { return "UNDEF"; }

  // Helper functions
  virtual void addFanout(CirGate* _g) { _fanoutList.push_back(_g); }

  virtual size_t getFanoutSize() const { return _fanoutList.size(); }
  virtual CirGate* getFanoutList(size_t _ith) const {
    assert(_ith < _fanoutList.size()); return _fanoutList[_ith];
  }
  virtual bool removeFanoutGate(const CirGate* _g) {
    bool _exist = false;
    size_t i = 0;
    while(i < _fanoutList.size()){
      if(_fanoutList[i] == _g){ _fanoutList.erase(_fanoutList.begin() + i); _exist = true; break; }
      else ++i;
    } return _exist;
  }
  // virtual void sortFanout() { sort(_fanoutList.begin(), _fanoutList.end(), GateCmp); }

  // Printing functions
  virtual void printGate() const;
  virtual void reportGate() const;
  virtual void reportFanin(int level) const;
  // virtual void reportFanout(int level) const;

private:
  GateList _fanoutList;             // Fan out list
};

#endif // CIR_GATE_H
