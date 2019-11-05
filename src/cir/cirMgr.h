/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <utility>

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"
#include "cirGate.h"

extern CirMgr *cirMgr;

typedef unordered_map<HashKey,  size_t, HashKeyH,  HashKeyE>         UMap;     // for strash
typedef set<CirGate*, GateCmp>                                   FECgroup;     // for FEC group
typedef set<CirGate*, GateCmp>                                    GateSet;     // for Floating list
typedef unordered_map<SimValue, FECgroup*, SimValueH, SimValueE>     SMap;     // for simulation
typedef vector<FECgroup*>                                       FECgroupS;     // for FEC groups
typedef vector<SimValue>                                     SimValuePack;     // a pack of SimValue (for PIs)
typedef vector<SimValuePack*>                               SimValuePackS;     // a list of SimValuePack
typedef pair<CirGate* , CirGate* >                               GatePair;     // a pair of gates

const size_t                                            SimLimit =  90000;     // Limit of Simulation patterns
const size_t                                              ILimit =     16;     // Limit of _I for simulating all paterns

class CirMgr
{
public:
   CirMgr(): _M(0), _I(0), _L(0), _O(0), _N(0), _nowPack(0), _packs(0), _fraiged(false) {
      _DFSflag = new unsigned;
      *(_DFSflag) = 0;
   }
   ~CirMgr() {
      // delete all gates in _totalList
      clear();
   }

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const {
    if(gid > _M + _O) return 0;
    
    return _totalList[gid];
  }

   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Member functions about circuit optimization
   void sweep();
   void optimize();

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }

   // Member functions about fraig
   void strash();
   void printFEC(size_t id) const;
   void fraig();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printFECPairs() const;
   void writeAag(ostream&) const;
   void writeGate(ostream&, CirGate*) const;

private:
   ofstream           *_simLog;

   unsigned _M;                        // Maximum variable index
   unsigned _I;                        // Inputs
   unsigned _L;                        // Latches
   unsigned _O;                        // Outputs
   unsigned _A;                        // AND Gate
   GateList _totalList;                // List of all gates, order : CONST0, 1 ~ _M, _M+1 ~ _M+_O
   GateList _PIList;                   // List of all PIs
   GateList _POList;                   // List of all POs
   GateSet _floatList1;                // (b) + (c) + (d) + PO with UNDEF fanin
   GateSet _floatList2;                // (a) + (c) + PI with no fanout
   GateList _netList;                  // Topological order of circuit
   unsigned _N;                        // Number of AIGs in _netList

   unsigned* _DFSflag;                 // Flag for DFS, counts from 0. For the next DFS, _DFSFlag++

   FECgroupS _fecGroupList;            // List of FEC groups
   SimValuePackS _nowPackList;         // SimValue packs
   SimValuePack* _nowPack;             // current SimValue pack, new simvalue will be stored in here
   size_t _packs;                      // Exact number of packs in _nowPackList

   SimValuePackS _goodPackList;        // good packs
   
   bool _fraiged;                      // Check if run through fraig

   // DFS purpose:
   void updateFlag() const { (*_DFSflag)++; }
   void updateAllFlag() const;
   unsigned currentFlag() const { return *_DFSflag; }
   void netlistDFS(CirGate* nowGate, unsigned& depth) const;
   void buildNetlist();
   void buildNetlistDFS(CirGate* nowGate);
   void updateFloating();

   // Optimize purpose:
   void replaceGate(CirGate* nowGate, CirGate* newGate, bool _inv);
   void deleteGate(CirGate* nowGate);
   void optMessage(unsigned newID, bool newInv, unsigned oldID) const;
   
   // Strash purpose:
   void mergeGate(CirGate* tarGate, CirGate* uGate, const bool _inv);

   // Simulation purpose:
   void buildFECgroupList();
   size_t FECgroupSize() const;
   void SVgenerator_all();
   void SVgenerator_1();
   void clearNowPackList();
   void recycleNowPackList();
   bool stopFunction();

   void SimulateAll();
   void SimulateList(const size_t _ith);
   void updateFECgroups();

   void resetNowPack();
   void reportSimLog(const size_t patternsN) const;

   void sortFECList();
   void MergeSort(size_t _l, size_t _r);
   void Merge(size_t _l, size_t _m, size_t _r);

   // Fraig purpose:
   void genProofModel(SatSolver& s);
   size_t proveSmall(SatSolver& s);
   size_t proveSmall3(SatSolver& s);
   size_t proveLarge(SatSolver& s, const size_t _lim1, const size_t _lim2);
   inline bool isFloating(CirGate* _g) const;

   bool proveZero(SatSolver& s, CirGate* _g, const bool _inv);
   bool proveGates(SatSolver& s, CirGate* _g1, CirGate* _g2, const bool _inv);

   void collectSimValue(SatSolver& s);
   void reSimulate();

   size_t MergeAll();
   void fraigMessage(unsigned newID, bool newInv, unsigned oldID) const;



   void clear(){
      delete _DFSflag;
      _DFSflag = 0;
      if(_M == 0) return;
      for(size_t i = 0; i <= _M + _O; ++i){
         if(_totalList[i]){
            delete _totalList[i];
            _totalList[i] = 0;
         }
      }
      for(size_t i = 0; i < _fecGroupList.size(); ++i){
         if(_fecGroupList[i])
            delete _fecGroupList[i];
      }
      for(size_t i = 0; i < _nowPackList.size(); ++i){
         if(_nowPackList[i])
            delete _nowPackList[i];
      }
      for(size_t i = 0; i < _goodPackList.size(); ++i){
         if(_goodPackList[i])
            delete _goodPackList[i];
      }
      if(_nowPack) delete _nowPack;
      _nowPack = 0;
      _packs = 0;
      _totalList.clear();
      _PIList.clear();
      _POList.clear();
      _floatList1.clear();
      _floatList2.clear();
      _netList.clear();
      _fecGroupList.clear();
      _nowPackList.clear();
      _goodPackList.clear();
      _M = _I = _L = _O = _A = _N = 0;
      _fraiged = false;
  }
};

#endif // CIR_MGR_H
