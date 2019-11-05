/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include <sstream>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
// static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine const (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}

static void reset_err() {
   // Reset variables
   // ======================
   lineNo = errInt = colNo = 0;
   errMsg = "";
   errGate = 0;
   // ======================
}

/*bool FECgroupCmp(FECgroup* _g1, FECgroup* _g2){
   FECgroup::iterator it1 = _g1->begin();
   FECgroup::iterator it2 = _g2->begin();

   return ((*it1)->getGateIndex()) < ((*it2)->getGateIndex());
}*/

/*struct FECgroupCMP{
   bool operator () (FECgroup* &_g1, FECgroup* &_g2) const {
      FECgroup::iterator it1 = (*_g1).begin();
      FECgroup::iterator it2 = (*_g2).begin();

      return ((*it1)->getGateIndex()) < ((*it2)->getGateIndex());
   }
}MYCMP;*/

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
bool
CirMgr::readCircuit(const string& fileName)
{
   // if(_M > 0) return false; // already read in

   /*
   cout << "Size : " << endl;
   cout << "String       : " << sizeof(string) << endl;
   cout << "Unsigned     : " << sizeof(unsigned) << endl;
   cout << "Size_t       : " << sizeof(size_t) << endl;
   //cout << "Bitset       : " << sizeof(bitset<2>) << endl;
   cout << endl;
   
   cout << "CirMgr       : " << sizeof(CirMgr) << endl;
   cout << "CirGate ptr  : " << sizeof(CirGate*) << endl;
   cout << "GateList     : " << sizeof(GateList) << endl;
   cout << "CirGate      : " << sizeof(CirGate) << endl;
   cout << "CirGatePI    : " << sizeof(CirGatePI) << endl;
   cout << "CirGatePO    : " << sizeof(CirGatePO) << endl;
   cout << "CirGateAIG   : " << sizeof(CirGateAIG) << endl;
   cout << "CirGateCONST : " << sizeof(CirGateCONST) << endl;
   cout << "CirGateUNDEF : " << sizeof(CirGateUNDEF) << endl;
   */

   ifstream f(fileName);
   // f.open(fileName, ifstream::in);
   if(!f.is_open()){
      // Cannot open file
      cout << "Cannot open design \"" << fileName << "\"!!" << endl;
      // f.close();
      return false;
   }

   // STEP 1 =====================================
   // Read in all text first
   vector<string> _inputLine;
   while(f.peek() != EOF){
      string str;
      getline(f,str);
      _inputLine.push_back(str);
   }
   f.close();

   reset_err();
   
   // STEP 2 =====================================
   // Get first line
   if(_inputLine[0].find_first_not_of(' ', 0) > 0){
      return parseError(EXTRA_SPACE);
   }
   colNo = _inputLine[0].find_first_of(' ', 0);
   errMsg = _inputLine[0].substr(0, colNo - 0);
   if(errMsg != "aag"){
      return parseError(ILLEGAL_IDENTIFIER);
   } errMsg = "";

   stringstream ss;
   ss << _inputLine[0];
   ss >> errMsg; // aag
   colNo = _inputLine[0].size();
   if(ss.peek() != EOF) ss >> _M;
   else { errMsg = "Missing number of variables"; return parseError(MISSING_NUM); }
   if(ss.peek() != EOF) ss >> _I;
   else { errMsg = "Missing number of PIs";       return parseError(MISSING_NUM); }
   if(ss.peek() != EOF) ss >> _L;
   else { errMsg = "Missing number of latches";   return parseError(MISSING_NUM); }
   if(ss.peek() != EOF) ss >> _O;
   else { errMsg = "Missing number of POs";       return parseError(MISSING_NUM); }
   if(ss.peek() != EOF) ss >> _A;
   else { errMsg = "Missing number of AIGs";      return parseError(MISSING_NUM); }
   if(ss.peek() != EOF){
      colNo = ss.tellg(); // ??
      return parseError(MISSING_NEWLINE); 
   }

   if(_M < _I + _L + _A){
      errMsg = "Number of variables";
      errInt = _M;
      return parseError(NUM_TOO_SMALL);
   }

   //cout << _M << ' ' << _I << ' ' << _L << ' ' << _O << ' ' << _A << endl;

   /*ss.str("");
   ss.clear();*/
   reset_err();
   // REMEMBER TO CLEAR ALL DATA BEFORE RETURN FALSE !!!!!
   // Umm... It seems that cirCmd.cpp already done this...
   

   
   // STEP 3 =====================================
   // Create all PIs

   size_t SSZ = _inputLine.size();
   // Create _totalList
   for(size_t i = 0; i <= _M + _O; ++i){
      _totalList.push_back(0);
   } // 1 + _M + _O elements

   // Create CONST0
   _totalList[0] = new CirGateCONST();
   unsigned _gateID;
   unsigned _halfGateID;
   for(lineNo = 1; lineNo <= _I; ++lineNo){
      ss.str("");
      ss.clear();
      ss << _inputLine[lineNo];

      char _peek = ss.peek();
      //cerr << "ss.peek() = \'" << _peek << "\'" << endl;
      if(_peek == EOF || _peek == ' '){
         errMsg = "PI";
         return parseError(MISSING_DEF); 
      }
      
      ss >> _gateID;
      if(_gateID == 0 || _gateID == 1){
         errInt = _gateID;
         return parseError(REDEF_CONST);
      }
      if(_gateID & 1){ // only handle integer, but not characters
         errMsg = "PI";
         errInt = _gateID;
         return parseError(CANNOT_INVERTED);
      }
      if(_gateID > (_M << 1)){ // PIs are all even -> max = _M*2
         errInt = _gateID;
         return parseError(MAX_LIT_ID);
      }
      // char _peek = ss.peek();
      /*_peek = ss.peek();
      cerr << "ss.peek() = \'" << _peek << "\'" << endl;*/
      if(ss.peek() != EOF){
         colNo = ss.tellg(); // ??
         return parseError(MISSING_NEWLINE); 
      }
      
      if(_totalList[_gateID >> 1] != 0){
         errGate = _totalList[_gateID >> 1];
         errInt = _gateID >> 1;
         return parseError(REDEF_GATE);
      }
      _halfGateID = _gateID >> 1;
      CirGate* newGate = new CirGatePI( _halfGateID, lineNo + 1);

      _PIList.push_back(newGate);
      _totalList[_halfGateID] = newGate;
   }
   reset_err();


   // STEP 4 =====================================
   // Create all POs

   for(lineNo = 1; lineNo <= _O; ++lineNo){
      CirGate* newGate = new CirGatePO( _M + lineNo, lineNo + _I + 1);
      // Not created fanin of PO yet !!!
      _POList.push_back(newGate);
      _totalList[_M + lineNo] = newGate;
   }
   // reset_err();


   // STEP 5 =====================================
   // Create all known AIGs

   for(lineNo = _I + _O + 1; lineNo <= _I + _O + _A; ++lineNo){
      ss.str("");
      ss.clear();
      ss << _inputLine[lineNo];
      if(!isdigit(ss.peek())){
         errMsg = "AIG";
         return parseError(MISSING_DEF);
      }
      
      ss >> _gateID;
      if(_gateID == 0 || _gateID == 1){
         errInt = _gateID;
         return parseError(REDEF_CONST);
      }
      if(_gateID > (_M << 1)){ // AIGs max = _M*2
         errInt = _gateID;
         return parseError(MAX_LIT_ID);
      }
      if(_gateID & 1){ // only handle integer, but not characters
         errMsg = "AIG gate";
         errInt = _gateID;
         return parseError(CANNOT_INVERTED);
      }

      _halfGateID = _gateID >> 1;
      if(_totalList[_halfGateID] != 0){ // defined
         errGate = _totalList[_halfGateID];
         errInt = _halfGateID;
         return parseError(REDEF_GATE);
      }
   
      CirGate* newGate = new CirGateAIG( _halfGateID, lineNo + 1);
      // Not created fanin of AIG yet !!!
      // _POList.push_back(newGate);
      _totalList[_halfGateID] = newGate;
   }
   reset_err();

   // At this moment, we created all the defined gates.
   // Including all PIs, POs, and AIGs (with specified fanins)


   // STEP 6 =====================================
   // Link all AIGs

   for(lineNo = _I + _O + 1; lineNo <= _I + _O + _A; ++lineNo){
      ss.str("");
      ss.clear();
      ss << _inputLine[lineNo];

      unsigned _gateID, _fan1, _fan2;
      ss >> _gateID;
      if(ss.peek() == EOF){
         colNo = ss.tellg();
         errMsg = "space character";
         return parseError(MISSING_NUM);
      }
      ss >> _fan1; // Fanin 1
      if(_fan1 > (_M << 1) + 1){ // POs are all even -> max = _M*2 + 1
         errInt = _fan1;
         return parseError(MAX_LIT_ID);
      }
      if(ss.peek() == EOF){
         colNo = ss.tellg();
         errMsg = "space character";
         return parseError(MISSING_NUM);
      }
      
      ss >> _fan2; // Fanin 1
      if(_fan2 > (_M << 1) + 1){ // POs are all even -> max = _M*2 + 1
         errInt = _fan2;
         return parseError(MAX_LIT_ID);
      }
      if(ss.peek() != EOF){
         colNo = ss.tellg();
         return parseError(MISSING_NEWLINE);
      }
      
      // cerr << _gateID << ' ' << _fan1 << ' ' << _fan2 << endl;
      // _gateID, _fan1, _fan2 read in
      // Not concerning about _fan1 == _gateID
      CirGate* faninGate1 = 0;
      CirGate* faninGate2 = 0;
      
      if(_totalList[_fan1 >> 1] != 0){ // defined
         faninGate1 = _totalList[_fan1 >> 1];
      } else {
         faninGate1 = new CirGateUNDEF(_fan1 >> 1);
         _totalList[_fan1 >> 1] = faninGate1;
      }
      if(_totalList[_fan2 >> 1] != 0){ // defined
         faninGate2 = _totalList[_fan2 >> 1];
      } else {
         faninGate2 = new CirGateUNDEF(_fan2 >> 1);
         _totalList[_fan2 >> 1] = faninGate2;
      }

      _halfGateID = _gateID >> 1;
      _totalList[_halfGateID]->addInverted((_fan1 & 1) ? true : false);
      _totalList[_halfGateID]->addFanin(faninGate1);
      faninGate1->addFanout(_totalList[_halfGateID]);

      _totalList[_halfGateID]->addInverted((_fan2 & 1) ? true : false);
      _totalList[_halfGateID]->addFanin(faninGate2);
      faninGate2->addFanout(_totalList[_halfGateID]);
   }
   reset_err();


   // STEP 7 =====================================
   // Create gates which are connected to POs

   for(lineNo = _I + 1; lineNo <= _I + _O; ++lineNo){
      ss.str("");
      ss.clear();
      ss << _inputLine[lineNo];

      ss >> _gateID;
      /*if(_gateID == 0 || _gateID == 1){
         errInt = _gateID;
         return parseError(REDEF_CONST);
      }*/
      if(_gateID > (_M << 1) + 1){ // POs are all even -> max = _M*2 + 1
         errInt = _gateID;
         return parseError(MAX_LIT_ID);
      }
      char _peek = ss.peek();
      if(_peek != EOF){
         colNo = ss.tellg(); // ??
         return parseError(MISSING_NEWLINE);
      }

      _halfGateID = _gateID >> 1;
      if(_totalList[_halfGateID] != 0){ // defined
         _totalList[_halfGateID]->addFanout(_POList[lineNo - _I - 1]);
         _POList[lineNo - _I - 1]->addFanin(_totalList[_gateID >> 1]);
         _POList[lineNo - _I - 1]->addInverted((_gateID & 1) ? true : false);
      } else {
         CirGate* newGate = new CirGateUNDEF( _halfGateID);
         _totalList[_halfGateID] = newGate;

         _totalList[_halfGateID]->addFanout(_POList[lineNo - _I - 1]);
         _POList[lineNo - _I - 1]->addFanin(_totalList[_halfGateID]);
         _POList[lineNo - _I - 1]->addInverted((_gateID & 1) ? true : false);
      }
   }
   reset_err();
   // PO done !


   // STEP 8 =====================================
   // Get symbols

   for(lineNo = _I + _O + _A + 1; lineNo < SSZ; ++lineNo){
      ss.str("");
      ss.clear();
      ss << _inputLine[lineNo];

      string _str = "";
      ss >> _str;
      if(_str == "c") break;
      if(_str[0] != 'i' && _str[0] != 'o'){
         break; // error
      }

      int _pos = 0;
      if(_str[0] == 'i'){
         if(!myStr2Int(_str.substr(1,_str.length() - 1), _pos)){
            break; // error
         }
         if((size_t)_pos >= _PIList.size()){
            errInt = _pos;
            errMsg = "PI index";
            return parseError(NUM_TOO_BIG);
         }
      }
      else if(_str[0] == 'o'){
         if(!myStr2Int(_str.substr(1,_str.length() - 1), _pos)){
            break; // error
         }
         if((size_t)_pos >= _POList.size()){
            errInt = _pos;
            errMsg = "PO index";
            return parseError(NUM_TOO_BIG);
         }
      }

      if(ss.peek() == EOF){
         errMsg = "symbolic name";
         return parseError(MISSING_IDENTIFIER);
      }
      // if redefined error is here

      string nameStr = _inputLine[lineNo].substr((int)ss.tellg() + 1, _inputLine[lineNo].length() - ss.tellg());
      // cout << "name : " << nameStr << endl;
      if(_str[0] == 'i'){
         _PIList[_pos]->modifyName(nameStr);
      }
      else if(_str[0] == 'o'){
         _POList[_pos]->modifyName(nameStr);
      }
   }
   reset_err();



   // STEP 9 =====================================
   // Build netlist

   // updateAllFlag();
   buildNetlist();


   // STEP 10 =====================================
   // Find all floating gates
   updateFloating();

   // sort(_floatList1.begin(), _floatList1.end(), GateCmp);
   // sort(_floatList2.begin(), _floatList2.end(), GateCmp);
   // sortFanout();

   reset_err();

   

   return true;
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
   cout << endl;
   cout << "Circuit Statistics" << endl;
   cout << "==================" << endl;
   cout << setw(7) << left << "  PI"    << setw(9) << right << _I << endl;
   cout << setw(7) << left << "  PO"    << setw(9) << right << _O << endl;
   cout << setw(7) << left << "  AIG"   << setw(9) << right << _A << endl;
   cout << "------------------" << endl;
   cout << setw(7) << left << "  Total" << setw(9) << right << _I + _O + _A << endl;
}

void
CirMgr::printNetlist() const
{
   cout << endl;
   /*updateAllFlag();
   updateFlag();
   unsigned nowDepth = 0;
   for(size_t i = 0; i < _O; ++i){
      netlistDFS(_POList[i], nowDepth);
   }*/
   for(size_t i = 0; i < _netList.size(); ++i){
      cout << "[" << i << "] ";
      _netList[i]->printGate();
      cout << endl;
   }
}

/*void
CirMgr::printNetlist() const
{

   cout << endl;
   for (unsigned i = 0, n = _dfsList.size(); i < n; ++i) {
      cout << "[" << i << "] ";
      _dfsList[i]->printGate();
   }

}*/

void
CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
   for(size_t i = 0; i < _I; ++i){
      cout << " " << _PIList[i]->getGateIndex() ;
   }
   cout << endl;
}

void
CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
   for(size_t i = 0; i < _O; ++i){
      cout << " " << _POList[i]->getGateIndex() ;
   }
   cout << endl;
}

void
CirMgr::printFloatGates() const
{
   if(!_floatList1.empty()){
      cout << "Gates with floating fanin(s):";
      GateSet::iterator it = _floatList1.begin();
      for( ; it != _floatList1.end(); ++it){
         cout << " " << (*it)->getGateIndex();
      }
      cout << endl;
   }
   if(!_floatList2.empty()){
      cout << "Gates defined but not used  :";
      GateSet::iterator it = _floatList2.begin();
      for( ; it != _floatList2.end(); ++it){
         cout << " " << (*it)->getGateIndex();
      }
      cout << endl;
   }
}

void
CirMgr::printFECPairs() const
{
   /*sort(_fecGroupList.begin(), _fecGroupList.end(), [](size_t &a, size_t &b){ 
      (*((FECgroup*)a)->begin())->getGateIndex() < (*((FECgroup*)b)->begin())->getGateIndex(); });*/
   // sortFECList();

   for(size_t i = 0; i < _fecGroupList.size(); ++i){
      FECgroup::iterator it = (_fecGroupList[i])->begin();
      SimValue _SV1 = (*it)->getSimValue();
      cout << "[" << i << "]";

      for( ; it != (_fecGroupList[i])->end(); ++it){
         cout << ' ';
         if((*it)->getSimValue() != _SV1) cout << "!";
         cout << (*it)->getGateIndex();
      }
      cout << endl;
   }
}

void
CirMgr::writeAag(ostream& outfile) const
{
   outfile << "aag " << _M << " " << _I << " " << _L << " " << _O << " " << _N << endl;
   for(size_t i = 0; i < _I; ++i){
      outfile << ((_PIList[i]->getGateIndex()) << 1) << endl;
   }
   for(size_t i = 0; i < _O; ++i){
      outfile << ((_POList[i]->getFaninList(0)->getGateIndex()) << 1) +
         ((_POList[i]->getInvertedList(0)) ? 1 : 0) << endl;
   }
   for(size_t i = 0; i < _netList.size(); ++i){
      if(_netList[i]->getTypeStr() == "AIG"){
         outfile << (_netList[i]->getGateIndex() << 1);
         outfile << " " << (_netList[i]->getFaninList(0)->getGateIndex() << 1) + (_netList[i]->getInvertedList(0)? 1 : 0);
         outfile << " " << (_netList[i]->getFaninList(1)->getGateIndex() << 1) + (_netList[i]->getInvertedList(1)? 1 : 0);
         outfile << endl;
      }
   }
   for(size_t i = 0; i < _I; ++i){
      if(_PIList[i]->getName() != ""){
         outfile << "i" << i << " " << _PIList[i]->getName() << endl;
      }
   }
   for(size_t i = 0; i < _O; ++i){
      if(_POList[i]->getName() != ""){
         outfile << "o" << i << " " << _POList[i]->getName() << endl;
      }
   }
   outfile << "c" << endl;
   outfile << "AAG output by Heng-Jui (Harry) Chang" << endl;
   /*outfile << "I love DSnP !!!" << endl;
   outfile << "AAG output by Harry Chang" << endl;
   outfile << "107-1 NTUEE DSnP" << endl;*/
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
}

/**********************************************************/
/*        class CirMgr private helper functions           */
/**********************************************************/

void CirMgr::updateAllFlag() const {
   for(size_t i = 0; i <= _M + _O; ++i){
      if(_totalList[i]){
         // if(_totalList[i]->getTypeStr() != "UNDEF"){
            while(_totalList[i]->getFlag() < currentFlag()){
               _totalList[i]->newFlag();
            }
         // }
      }
   }
}

void CirMgr::netlistDFS(CirGate* nowGate, unsigned& depth) const {
   if(nowGate == 0) return;
   if(nowGate->getTypeStr() == "UNDEF") return;
   
   if(nowGate->getFlag() == currentFlag()) return; // visited
   
   nowGate->newFlag(); // Update flag
   if(nowGate->getTypeStr() == "PO"){
      netlistDFS(nowGate->getFaninList(0), depth);
   }
   else if(nowGate->getTypeStr() == "AIG"){
      netlistDFS(nowGate->getFaninList(0), depth);
      netlistDFS(nowGate->getFaninList(1), depth);
   }
   cout << "[" << depth << "] ";
   nowGate->printGate();
   cout << endl;
   ++depth;
   return;
}

void CirMgr::buildNetlistDFS(CirGate* nowGate){
   if(nowGate == 0) return;
   if(nowGate->getTypeStr() == "UNDEF") return;
   if(nowGate->getFlag() == currentFlag()) return; // visited
   
   nowGate->newFlag(); // Update flag
   if(nowGate->getTypeStr() == "PO"){
      buildNetlistDFS(nowGate->getFaninList(0));
   }
   else if(nowGate->getTypeStr() == "AIG"){
      ++_N;
      buildNetlistDFS(nowGate->getFaninList(0));
      buildNetlistDFS(nowGate->getFaninList(1));
   }
   _netList.push_back(nowGate);
   // Record the topological order of gates
   nowGate->modifyTopOrder(_netList.size());
   return;
}

void CirMgr::buildNetlist() {
   _netList.clear();
   updateFlag();
   _N = 0;
   for(size_t i = 0; i < _O; ++i){
      buildNetlistDFS(_POList[i]);
   }
   updateAllFlag();

   _A = 0;
   for(size_t i = 1; i <= _M; ++i){
      if(_totalList[i] != 0){
         if(_totalList[i]->isAig())
            ++_A;
      }
   }
}

void CirMgr::updateFloating(){
   _floatList1.clear();
   _floatList2.clear();
   // _floatList1 : (b) + (c) + (d) + PO with UNDEF fanin
   // _floatList2 : (a) + (c) + PI with no fanout
   for(size_t i = 1; i <= _M; ++i){
      if(_totalList[i] == 0) continue;

      // (b)
      // gates that cannot be reached from any PI
      if(_totalList[i]->getTypeStr() == "UNDEF"){
         // _floatList1.insert(_totalList[i]);
         continue;
      }
      // PI with no fanout
      if((_totalList[i]->getTypeStr() == "PI")){
         if(_totalList[i]->getFanoutSize() == 0){
            _floatList2.insert(_totalList[i]);
         }
         continue;
      }
      // (c)
      // a gate that cannot be reached from PI and PO
      if(_totalList[i]->getFaninSize() == 0 && _totalList[i]->getFanoutSize() == 0){
         _floatList1.insert(_totalList[i]);
         _floatList2.insert(_totalList[i]);
         continue;
      }
      // (a)
      // a gate that cannot reach any PO
      if(_totalList[i]->getFanoutSize() == 0){
         _floatList2.insert(_totalList[i]);
         continue;
      }
      // (b) + (d)
      // d : a gate  with a floating fanin
      if((_totalList[i]->getFaninList(0))->getTypeStr() == "UNDEF" || 
         (_totalList[i]->getFaninList(1))->getTypeStr() == "UNDEF")
      {
         _floatList1.insert(_totalList[i]);
      }
   }

   // PO with fanin undefined
   for(size_t i = 0; i < _O; ++i){
      if(_POList[i]->getFaninList(0)->getTypeStr() == "UNDEF"){
         _floatList1.insert(_POList[i]);
      }
   }
}