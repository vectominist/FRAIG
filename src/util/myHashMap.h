/****************************************************************************
  FileName     [ myHashMap.h ]
  PackageName  [ util ]
  Synopsis     [ Define HashMap and Cache ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2009-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_MAP_H
#define MY_HASH_MAP_H

#include <vector>

using namespace std;

// TODO: (Optionally) Implement your own HashMap and Cache classes.

//-----------------------
// Define HashMap classes
//-----------------------
// To use HashMap ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//

/**********************************************************/
/*                Hash Key for Strash                     */
/**********************************************************/

class HashKey
{
public:
   HashKey(): in0(0), in1(0) {}
   HashKey(size_t _g0, const bool _inv0, size_t _g1, const bool _inv1) {
      in0 = _g0;
      in1 = _g1;
      // If inverted : LSB + 1
      if(_inv0) in0++;
      if(_inv1) in1++;
   }

   size_t operator() () const { return in0 ^ in1; }

   bool operator == (const HashKey& k) const {
      if((in0 == k.in0) && (in1 == k.in1)){
         return true;
      }
      if((in0 == k.in1) && (in1 == k.in0)){
         return true;
      }
      return false;
   }

private:
   size_t in0, in1;
};
struct HashKeyH {
   // hash function of hashkey
   size_t operator() (const HashKey& k) const {
      return k();
   }
};
struct HashKeyE {
   // equal function of hashkey
   bool operator() (const HashKey& k1, const HashKey& k2) const {
      return (k1 == k2);
   }
};

/**********************************************************/
/*             Simulate Value for Simulation              */
/**********************************************************/
const size_t ONE = 1;
class SimValue
{
public:
   SimValue(): _val(0) {}
   SimValue(size_t _inval): _val(_inval) {}
   SimValue(const SimValue& _newSV) { _val = _newSV._val; }

   size_t operator() () const { return (_val & ONE) ? _val : ~_val; }
   // Hash function :
   // if LSB of _val is 1  ->  return  _val  (original)
   // if LSB of _val is 0  ->  return ~_val  (inverted)
   
   // Advantage : inverted identical will have the same hash value !

   bool operator == (const SimValue& k) const {
      if((_val ==  k._val) || (_val == ~k._val)) return true;
      return false;
   }
   bool operator != (const SimValue& k) const {
      if(_val != k._val) return true;
      return false;
   }
   SimValue operator & (const SimValue& k) const {
      SimValue _tmp(_val & k._val);
      return _tmp;
   }
   SimValue operator ~ () const {
      SimValue _tmp(~_val);
      return _tmp;
   }

   SimValue& operator = (const SimValue& k) { _val = k._val; return *(this); }

   bool getBit(const size_t _i) const { return ((_val & (ONE << _i)) > 0) ? true : false; }

   void modify(const size_t _i, const bool _new) {
      if(((_val & (ONE << _i)) > 0) == _new) return;
      if(_new){ // 0 -> 1
         _val = (_val | (ONE << _i));
      } else {  // 1 -> 0
         _val = (_val ^ (ONE << _i));
      }
   }

   void printVal() const {
      for(size_t i = 0; i < 64; ++i){
         cout << ((_val & (ONE << i)) > 0 ? '1' : '0');
         if(((i + 1) % 8 == 0) && (i != 63)) cout << "_";
      }
   }

   void reset() { _val = 0; }

private:
   size_t _val;
};
struct SimValueH {
   // hash function of SimValue
   size_t operator() (const SimValue& k) const {
      return k();
   }
};
struct SimValueE {
   // equal function of SimValue
   bool operator() (const SimValue& k1, const SimValue& k2) const {
      return (k1 == k2);
   }
};


template <class HashKey, class HashData>
class HashMap
{
typedef pair<HashKey, HashData> HashNode;

public:
   HashMap(size_t b=0) : _numBuckets(0), _buckets(0) { if (b != 0) init(b); }
   ~HashMap() { reset(); }

   // [Optional] TODO: implement the HashMap<HashKey, HashData>::iterator
   // o An iterator should be able to go through all the valid HashNodes
   //   in the HashMap
   // o Functions to be implemented:
   //   - constructor(s), destructor
   //   - operator '*': return the HashNode
   //   - ++/--iterator, iterator++/--
   //   - operators '=', '==', !="
   //
   class iterator
   {
      friend class HashMap<HashKey, HashData>;

   public:

   private:
   };

   void init(size_t b) {
      reset(); _numBuckets = b; _buckets = new vector<HashNode>[b]; }
   void reset() {
      _numBuckets = 0;
      if (_buckets) { delete [] _buckets; _buckets = 0; }
   }
   void clear() {
      for (size_t i = 0; i < _numBuckets; ++i) _buckets[i].clear();
   }
   size_t numBuckets() const { return _numBuckets; }

   vector<HashNode>& operator [] (size_t i) { return _buckets[i]; }
   const vector<HashNode>& operator [](size_t i) const { return _buckets[i]; }

   // TODO: implement these functions
   //
   // Point to the first valid data
   iterator begin() const { return iterator(); }
   // Pass the end
   iterator end() const { return iterator(); }
   // return true if no valid data
   bool empty() const { return true; }
   // number of valid data
   size_t size() const { size_t s = 0; return s; }

   // check if k is in the hash...
   // if yes, return true;
   // else return false;
   bool check(const HashKey& k) const { return false; }

   // query if k is in the hash...
   // if yes, replace d with the data in the hash and return true;
   // else return false;
   bool query(const HashKey& k, HashData& d) const { return false; }

   // update the entry in hash that is equal to k (i.e. == return true)
   // if found, update that entry with d and return true;
   // else insert d into hash as a new entry and return false;
   bool update(const HashKey& k, HashData& d) { return false; }

   // return true if inserted d successfully (i.e. k is not in the hash)
   // return false is k is already in the hash ==> will not insert
   bool insert(const HashKey& k, const HashData& d) { return true; }

   // return true if removed successfully (i.e. k is in the hash)
   // return fasle otherwise (i.e. nothing is removed)
   bool remove(const HashKey& k) { return false; }

private:
   // Do not add any extra data member
   size_t                   _numBuckets;
   vector<HashNode>*        _buckets;

   size_t bucketNum(const HashKey& k) const {
      return (k() % _numBuckets); }

};


//---------------------
// Define Cache classes
//---------------------
// To use Cache ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
// class CacheKey
// {
// public:
//    CacheKey() {}
//    
//    size_t operator() () const { return 0; }
//   
//    bool operator == (const CacheKey&) const { return true; }
//       
// private:
// }; 
// 
template <class CacheKey, class CacheData>
class Cache
{
typedef pair<CacheKey, CacheData> CacheNode;

public:
   Cache() : _size(0), _cache(0) {}
   Cache(size_t s) : _size(0), _cache(0) { init(s); }
   ~Cache() { reset(); }

   // NO NEED to implement Cache::iterator class

   // TODO: implement these functions
   //
   // Initialize _cache with size s
   void init(size_t s) { reset(); _size = s; _cache = new CacheNode[s]; }
   void reset() {  _size = 0; if (_cache) { delete [] _cache; _cache = 0; } }

   size_t size() const { return _size; }

   CacheNode& operator [] (size_t i) { return _cache[i]; }
   const CacheNode& operator [](size_t i) const { return _cache[i]; }

   // return false if cache miss
   bool read(const CacheKey& k, CacheData& d) const {
      size_t i = k() % _size;
      if (k == _cache[i].first) {
         d = _cache[i].second;
         return true;
      }
      return false;
   }
   // If k is already in the Cache, overwrite the CacheData
   void write(const CacheKey& k, const CacheData& d) {
      size_t i = k() % _size;
      _cache[i].first = k;
      _cache[i].second = d;
   }

private:
   // Do not add any extra data member
   size_t         _size;
   CacheNode*     _cache;
};


#endif // MY_HASH_H
