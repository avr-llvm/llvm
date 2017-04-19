//===-- APInt.cpp - Implement APInt class ---------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a class to represent arbitrary precision integer
// constant values and provide a variety of arithmetic operations on them.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
using namespace llvm;

#define DEBUG_TYPE "apint"

/// A utility function for allocating memory, checking for allocation failures,
/// and ensuring the contents are zeroed.
inline static uint64_t* getClearedMemory(unsigned numWords) {
  uint64_t * result = new uint64_t[numWords];
  assert(result && "APInt memory allocation fails!");
  memset(result, 0, numWords * sizeof(uint64_t));
  return result;
}

/// A utility function for allocating memory and checking for allocation
/// failure.  The content is not zeroed.
inline static uint64_t* getMemory(unsigned numWords) {
  uint64_t * result = new uint64_t[numWords];
  assert(result && "APInt memory allocation fails!");
  return result;
}

/// A utility function that converts a character to a digit.
inline static unsigned getDigit(char cdigit, uint8_t radix) {
  unsigned r;

  if (radix == 16 || radix == 36) {
    r = cdigit - '0';
    if (r <= 9)
      return r;

    r = cdigit - 'A';
    if (r <= radix - 11U)
      return r + 10;

    r = cdigit - 'a';
    if (r <= radix - 11U)
      return r + 10;

    radix = 10;
  }

  r = cdigit - '0';
  if (r < radix)
    return r;

  return -1U;
}


void APInt::initSlowCase(uint64_t val, bool isSigned) {
  VAL = 0;
  pVal = getClearedMemory(getNumWords());
  pVal[0] = val;
  if (isSigned && int64_t(val) < 0)
    for (unsigned i = 1; i < getNumWords(); ++i)
      pVal[i] = -1ULL;
  clearUnusedBits();
}

void APInt::initSlowCase(const APInt& that) {
  VAL = 0;
  pVal = getMemory(getNumWords());
  memcpy(pVal, that.pVal, getNumWords() * APINT_WORD_SIZE);
}

void APInt::initFromArray(ArrayRef<uint64_t> bigVal) {
  assert(BitWidth && "Bitwidth too small");
  assert(bigVal.data() && "Null pointer detected!");
  if (isSingleWord())
    VAL = bigVal[0];
  else {
    // Get memory, cleared to 0
    VAL = 0;
    pVal = getClearedMemory(getNumWords());
    // Calculate the number of words to copy
    unsigned words = std::min<unsigned>(bigVal.size(), getNumWords());
    // Copy the words from bigVal to pVal
    memcpy(pVal, bigVal.data(), words * APINT_WORD_SIZE);
  }
  // Make sure unused high bits are cleared
  clearUnusedBits();
}

APInt::APInt(unsigned numBits, ArrayRef<uint64_t> bigVal)
  : BitWidth(numBits) {
  initFromArray(bigVal);
}

APInt::APInt(unsigned numBits, unsigned numWords, const uint64_t bigVal[])
  : BitWidth(numBits) {
  initFromArray(makeArrayRef(bigVal, numWords));
}

APInt::APInt(unsigned numbits, StringRef Str, uint8_t radix)
  : VAL(0), BitWidth(numbits) {
  assert(BitWidth && "Bitwidth too small");
  fromString(numbits, Str, radix);
}

void APInt::AssignSlowCase(const APInt& RHS) {
  // Don't do anything for X = X
  if (this == &RHS)
    return;

  if (BitWidth == RHS.getBitWidth()) {
    // assume same bit-width single-word case is already handled
    assert(!isSingleWord());
    memcpy(pVal, RHS.pVal, getNumWords() * APINT_WORD_SIZE);
    return;
  }

  if (isSingleWord()) {
    // assume case where both are single words is already handled
    assert(!RHS.isSingleWord());
    VAL = 0;
    pVal = getMemory(RHS.getNumWords());
    memcpy(pVal, RHS.pVal, RHS.getNumWords() * APINT_WORD_SIZE);
  } else if (getNumWords() == RHS.getNumWords())
    memcpy(pVal, RHS.pVal, RHS.getNumWords() * APINT_WORD_SIZE);
  else if (RHS.isSingleWord()) {
    delete [] pVal;
    VAL = RHS.VAL;
  } else {
    delete [] pVal;
    pVal = getMemory(RHS.getNumWords());
    memcpy(pVal, RHS.pVal, RHS.getNumWords() * APINT_WORD_SIZE);
  }
  BitWidth = RHS.BitWidth;
  clearUnusedBits();
}

/// This method 'profiles' an APInt for use with FoldingSet.
void APInt::Profile(FoldingSetNodeID& ID) const {
  ID.AddInteger(BitWidth);

  if (isSingleWord()) {
    ID.AddInteger(VAL);
    return;
  }

  unsigned NumWords = getNumWords();
  for (unsigned i = 0; i < NumWords; ++i)
    ID.AddInteger(pVal[i]);
}

/// @brief Prefix increment operator. Increments the APInt by one.
APInt& APInt::operator++() {
  if (isSingleWord())
    ++VAL;
  else
    tcIncrement(pVal, getNumWords());
  return clearUnusedBits();
}

/// @brief Prefix decrement operator. Decrements the APInt by one.
APInt& APInt::operator--() {
  if (isSingleWord())
    --VAL;
  else
    tcDecrement(pVal, getNumWords());
  return clearUnusedBits();
}

/// Adds the RHS APint to this APInt.
/// @returns this, after addition of RHS.
/// @brief Addition assignment operator.
APInt& APInt::operator+=(const APInt& RHS) {
  assert(BitWidth == RHS.BitWidth && "Bit widths must be the same");
  if (isSingleWord())
    VAL += RHS.VAL;
  else
    tcAdd(pVal, RHS.pVal, 0, getNumWords());
  return clearUnusedBits();
}

APInt& APInt::operator+=(uint64_t RHS) {
  if (isSingleWord())
    VAL += RHS;
  else
    tcAddPart(pVal, RHS, getNumWords());
  return clearUnusedBits();
}

/// Subtracts the RHS APInt from this APInt
/// @returns this, after subtraction
/// @brief Subtraction assignment operator.
APInt& APInt::operator-=(const APInt& RHS) {
  assert(BitWidth == RHS.BitWidth && "Bit widths must be the same");
  if (isSingleWord())
    VAL -= RHS.VAL;
  else
    tcSubtract(pVal, RHS.pVal, 0, getNumWords());
  return clearUnusedBits();
}

APInt& APInt::operator-=(uint64_t RHS) {
  if (isSingleWord())
    VAL -= RHS;
  else
    tcSubtractPart(pVal, RHS, getNumWords());
  return clearUnusedBits();
}

/// Multiplies an integer array, x, by a uint64_t integer and places the result
/// into dest.
/// @returns the carry out of the multiplication.
/// @brief Multiply a multi-digit APInt by a single digit (64-bit) integer.
static uint64_t mul_1(uint64_t dest[], uint64_t x[], unsigned len, uint64_t y) {
  // Split y into high 32-bit part (hy)  and low 32-bit part (ly)
  uint64_t ly = y & 0xffffffffULL, hy = y >> 32;
  uint64_t carry = 0;

  // For each digit of x.
  for (unsigned i = 0; i < len; ++i) {
    // Split x into high and low words
    uint64_t lx = x[i] & 0xffffffffULL;
    uint64_t hx = x[i] >> 32;
    // hasCarry - A flag to indicate if there is a carry to the next digit.
    // hasCarry == 0, no carry
    // hasCarry == 1, has carry
    // hasCarry == 2, no carry and the calculation result == 0.
    uint8_t hasCarry = 0;
    dest[i] = carry + lx * ly;
    // Determine if the add above introduces carry.
    hasCarry = (dest[i] < carry) ? 1 : 0;
    carry = hx * ly + (dest[i] >> 32) + (hasCarry ? (1ULL << 32) : 0);
    // The upper limit of carry can be (2^32 - 1)(2^32 - 1) +
    // (2^32 - 1) + 2^32 = 2^64.
    hasCarry = (!carry && hasCarry) ? 1 : (!carry ? 2 : 0);

    carry += (lx * hy) & 0xffffffffULL;
    dest[i] = (carry << 32) | (dest[i] & 0xffffffffULL);
    carry = (((!carry && hasCarry != 2) || hasCarry == 1) ? (1ULL << 32) : 0) +
            (carry >> 32) + ((lx * hy) >> 32) + hx * hy;
  }
  return carry;
}

/// Multiplies integer array x by integer array y and stores the result into
/// the integer array dest. Note that dest's size must be >= xlen + ylen.
/// @brief Generalized multiplication of integer arrays.
static void mul(uint64_t dest[], uint64_t x[], unsigned xlen, uint64_t y[],
                unsigned ylen) {
  dest[xlen] = mul_1(dest, x, xlen, y[0]);
  for (unsigned i = 1; i < ylen; ++i) {
    uint64_t ly = y[i] & 0xffffffffULL, hy = y[i] >> 32;
    uint64_t carry = 0, lx = 0, hx = 0;
    for (unsigned j = 0; j < xlen; ++j) {
      lx = x[j] & 0xffffffffULL;
      hx = x[j] >> 32;
      // hasCarry - A flag to indicate if has carry.
      // hasCarry == 0, no carry
      // hasCarry == 1, has carry
      // hasCarry == 2, no carry and the calculation result == 0.
      uint8_t hasCarry = 0;
      uint64_t resul = carry + lx * ly;
      hasCarry = (resul < carry) ? 1 : 0;
      carry = (hasCarry ? (1ULL << 32) : 0) + hx * ly + (resul >> 32);
      hasCarry = (!carry && hasCarry) ? 1 : (!carry ? 2 : 0);

      carry += (lx * hy) & 0xffffffffULL;
      resul = (carry << 32) | (resul & 0xffffffffULL);
      dest[i+j] += resul;
      carry = (((!carry && hasCarry != 2) || hasCarry == 1) ? (1ULL << 32) : 0)+
              (carry >> 32) + (dest[i+j] < resul ? 1 : 0) +
              ((lx * hy) >> 32) + hx * hy;
    }
    dest[i+xlen] = carry;
  }
}

APInt& APInt::operator*=(const APInt& RHS) {
  assert(BitWidth == RHS.BitWidth && "Bit widths must be the same");
  if (isSingleWord()) {
    VAL *= RHS.VAL;
    clearUnusedBits();
    return *this;
  }

  // Get some bit facts about LHS and check for zero
  unsigned lhsBits = getActiveBits();
  unsigned lhsWords = !lhsBits ? 0 : whichWord(lhsBits - 1) + 1;
  if (!lhsWords)
    // 0 * X ===> 0
    return *this;

  // Get some bit facts about RHS and check for zero
  unsigned rhsBits = RHS.getActiveBits();
  unsigned rhsWords = !rhsBits ? 0 : whichWord(rhsBits - 1) + 1;
  if (!rhsWords) {
    // X * 0 ===> 0
    clearAllBits();
    return *this;
  }

  // Allocate space for the result
  unsigned destWords = rhsWords + lhsWords;
  uint64_t *dest = getMemory(destWords);

  // Perform the long multiply
  mul(dest, pVal, lhsWords, RHS.pVal, rhsWords);

  // Copy result back into *this
  clearAllBits();
  unsigned wordsToCopy = destWords >= getNumWords() ? getNumWords() : destWords;
  memcpy(pVal, dest, wordsToCopy * APINT_WORD_SIZE);
  clearUnusedBits();

  // delete dest array and return
  delete[] dest;
  return *this;
}

void APInt::AndAssignSlowCase(const APInt& RHS) {
  tcAnd(pVal, RHS.pVal, getNumWords());
}

void APInt::OrAssignSlowCase(const APInt& RHS) {
  tcOr(pVal, RHS.pVal, getNumWords());
}

void APInt::XorAssignSlowCase(const APInt& RHS) {
  tcXor(pVal, RHS.pVal, getNumWords());
}

APInt APInt::operator*(const APInt& RHS) const {
  assert(BitWidth == RHS.BitWidth && "Bit widths must be the same");
  if (isSingleWord())
    return APInt(BitWidth, VAL * RHS.VAL);
  APInt Result(*this);
  Result *= RHS;
  return Result;
}

bool APInt::EqualSlowCase(const APInt& RHS) const {
  return std::equal(pVal, pVal + getNumWords(), RHS.pVal);
}

bool APInt::EqualSlowCase(uint64_t Val) const {
  unsigned n = getActiveBits();
  if (n <= APINT_BITS_PER_WORD)
    return pVal[0] == Val;
  else
    return false;
}

bool APInt::ult(const APInt& RHS) const {
  assert(BitWidth == RHS.BitWidth && "Bit widths must be same for comparison");
  if (isSingleWord())
    return VAL < RHS.VAL;

  // Get active bit length of both operands
  unsigned n1 = getActiveBits();
  unsigned n2 = RHS.getActiveBits();

  // If magnitude of LHS is less than RHS, return true.
  if (n1 < n2)
    return true;

  // If magnitude of RHS is greater than LHS, return false.
  if (n2 < n1)
    return false;

  // If they both fit in a word, just compare the low order word
  if (n1 <= APINT_BITS_PER_WORD && n2 <= APINT_BITS_PER_WORD)
    return pVal[0] < RHS.pVal[0];

  // Otherwise, compare all words
  unsigned topWord = whichWord(std::max(n1,n2)-1);
  for (int i = topWord; i >= 0; --i) {
    if (pVal[i] > RHS.pVal[i])
      return false;
    if (pVal[i] < RHS.pVal[i])
      return true;
  }
  return false;
}

bool APInt::slt(const APInt& RHS) const {
  assert(BitWidth == RHS.BitWidth && "Bit widths must be same for comparison");
  if (isSingleWord()) {
    int64_t lhsSext = SignExtend64(VAL, BitWidth);
    int64_t rhsSext = SignExtend64(RHS.VAL, BitWidth);
    return lhsSext < rhsSext;
  }

  bool lhsNeg = isNegative();
  bool rhsNeg = RHS.isNegative();

  // If the sign bits don't match, then (LHS < RHS) if LHS is negative
  if (lhsNeg != rhsNeg)
    return lhsNeg;

  // Otherwise we can just use an unsigned comparison, because even negative
  // numbers compare correctly this way if both have the same signed-ness.
  return ult(RHS);
}

void APInt::setBit(unsigned bitPosition) {
  if (isSingleWord())
    VAL |= maskBit(bitPosition);
  else
    pVal[whichWord(bitPosition)] |= maskBit(bitPosition);
}

void APInt::setBitsSlowCase(unsigned loBit, unsigned hiBit) {
  unsigned loWord = whichWord(loBit);
  unsigned hiWord = whichWord(hiBit);

  // Create an initial mask for the low word with zeros below loBit.
  uint64_t loMask = UINT64_MAX << whichBit(loBit);

  // If hiBit is not aligned, we need a high mask.
  unsigned hiShiftAmt = whichBit(hiBit);
  if (hiShiftAmt != 0) {
    // Create a high mask with zeros above hiBit.
    uint64_t hiMask = UINT64_MAX >> (APINT_BITS_PER_WORD - hiShiftAmt);
    // If loWord and hiWord are equal, then we combine the masks. Otherwise,
    // set the bits in hiWord.
    if (hiWord == loWord)
      loMask &= hiMask;
    else
      pVal[hiWord] |= hiMask;
  }
  // Apply the mask to the low word.
  pVal[loWord] |= loMask;

  // Fill any words between loWord and hiWord with all ones.
  for (unsigned word = loWord + 1; word < hiWord; ++word)
    pVal[word] = UINT64_MAX;
}

/// Set the given bit to 0 whose position is given as "bitPosition".
/// @brief Set a given bit to 0.
void APInt::clearBit(unsigned bitPosition) {
  if (isSingleWord())
    VAL &= ~maskBit(bitPosition);
  else
    pVal[whichWord(bitPosition)] &= ~maskBit(bitPosition);
}

/// @brief Toggle every bit to its opposite value.
void APInt::flipAllBitsSlowCase() {
  tcComplement(pVal, getNumWords());
  clearUnusedBits();
}

/// Toggle a given bit to its opposite value whose position is given
/// as "bitPosition".
/// @brief Toggles a given bit to its opposite value.
void APInt::flipBit(unsigned bitPosition) {
  assert(bitPosition < BitWidth && "Out of the bit-width range!");
  if ((*this)[bitPosition]) clearBit(bitPosition);
  else setBit(bitPosition);
}

void APInt::insertBits(const APInt &subBits, unsigned bitPosition) {
  unsigned subBitWidth = subBits.getBitWidth();
  assert(0 < subBitWidth && (subBitWidth + bitPosition) <= BitWidth &&
         "Illegal bit insertion");

  // Insertion is a direct copy.
  if (subBitWidth == BitWidth) {
    *this = subBits;
    return;
  }

  // Single word result can be done as a direct bitmask.
  if (isSingleWord()) {
    uint64_t mask = UINT64_MAX >> (APINT_BITS_PER_WORD - subBitWidth);
    VAL &= ~(mask << bitPosition);
    VAL |= (subBits.VAL << bitPosition);
    return;
  }

  unsigned loBit = whichBit(bitPosition);
  unsigned loWord = whichWord(bitPosition);
  unsigned hi1Word = whichWord(bitPosition + subBitWidth - 1);

  // Insertion within a single word can be done as a direct bitmask.
  if (loWord == hi1Word) {
    uint64_t mask = UINT64_MAX >> (APINT_BITS_PER_WORD - subBitWidth);
    pVal[loWord] &= ~(mask << loBit);
    pVal[loWord] |= (subBits.VAL << loBit);
    return;
  }

  // Insert on word boundaries.
  if (loBit == 0) {
    // Direct copy whole words.
    unsigned numWholeSubWords = subBitWidth / APINT_BITS_PER_WORD;
    memcpy(pVal + loWord, subBits.getRawData(),
           numWholeSubWords * APINT_WORD_SIZE);

    // Mask+insert remaining bits.
    unsigned remainingBits = subBitWidth % APINT_BITS_PER_WORD;
    if (remainingBits != 0) {
      uint64_t mask = UINT64_MAX >> (APINT_BITS_PER_WORD - remainingBits);
      pVal[hi1Word] &= ~mask;
      pVal[hi1Word] |= subBits.getWord(subBitWidth - 1);
    }
    return;
  }

  // General case - set/clear individual bits in dst based on src.
  // TODO - there is scope for optimization here, but at the moment this code
  // path is barely used so prefer readability over performance.
  for (unsigned i = 0; i != subBitWidth; ++i) {
    if (subBits[i])
      setBit(bitPosition + i);
    else
      clearBit(bitPosition + i);
  }
}

APInt APInt::extractBits(unsigned numBits, unsigned bitPosition) const {
  assert(numBits > 0 && "Can't extract zero bits");
  assert(bitPosition < BitWidth && (numBits + bitPosition) <= BitWidth &&
         "Illegal bit extraction");

  if (isSingleWord())
    return APInt(numBits, VAL >> bitPosition);

  unsigned loBit = whichBit(bitPosition);
  unsigned loWord = whichWord(bitPosition);
  unsigned hiWord = whichWord(bitPosition + numBits - 1);

  // Single word result extracting bits from a single word source.
  if (loWord == hiWord)
    return APInt(numBits, pVal[loWord] >> loBit);

  // Extracting bits that start on a source word boundary can be done
  // as a fast memory copy.
  if (loBit == 0)
    return APInt(numBits, makeArrayRef(pVal + loWord, 1 + hiWord - loWord));

  // General case - shift + copy source words directly into place.
  APInt Result(numBits, 0);
  unsigned NumSrcWords = getNumWords();
  unsigned NumDstWords = Result.getNumWords();

  for (unsigned word = 0; word < NumDstWords; ++word) {
    uint64_t w0 = pVal[loWord + word];
    uint64_t w1 =
        (loWord + word + 1) < NumSrcWords ? pVal[loWord + word + 1] : 0;
    Result.pVal[word] = (w0 >> loBit) | (w1 << (APINT_BITS_PER_WORD - loBit));
  }

  return Result.clearUnusedBits();
}

unsigned APInt::getBitsNeeded(StringRef str, uint8_t radix) {
  assert(!str.empty() && "Invalid string length");
  assert((radix == 10 || radix == 8 || radix == 16 || radix == 2 ||
          radix == 36) &&
         "Radix should be 2, 8, 10, 16, or 36!");

  size_t slen = str.size();

  // Each computation below needs to know if it's negative.
  StringRef::iterator p = str.begin();
  unsigned isNegative = *p == '-';
  if (*p == '-' || *p == '+') {
    p++;
    slen--;
    assert(slen && "String is only a sign, needs a value.");
  }

  // For radixes of power-of-two values, the bits required is accurately and
  // easily computed
  if (radix == 2)
    return slen + isNegative;
  if (radix == 8)
    return slen * 3 + isNegative;
  if (radix == 16)
    return slen * 4 + isNegative;

  // FIXME: base 36

  // This is grossly inefficient but accurate. We could probably do something
  // with a computation of roughly slen*64/20 and then adjust by the value of
  // the first few digits. But, I'm not sure how accurate that could be.

  // Compute a sufficient number of bits that is always large enough but might
  // be too large. This avoids the assertion in the constructor. This
  // calculation doesn't work appropriately for the numbers 0-9, so just use 4
  // bits in that case.
  unsigned sufficient
    = radix == 10? (slen == 1 ? 4 : slen * 64/18)
                 : (slen == 1 ? 7 : slen * 16/3);

  // Convert to the actual binary value.
  APInt tmp(sufficient, StringRef(p, slen), radix);

  // Compute how many bits are required. If the log is infinite, assume we need
  // just bit.
  unsigned log = tmp.logBase2();
  if (log == (unsigned)-1) {
    return isNegative + 1;
  } else {
    return isNegative + log + 1;
  }
}

hash_code llvm::hash_value(const APInt &Arg) {
  if (Arg.isSingleWord())
    return hash_combine(Arg.VAL);

  return hash_combine_range(Arg.pVal, Arg.pVal + Arg.getNumWords());
}

bool APInt::isSplat(unsigned SplatSizeInBits) const {
  assert(getBitWidth() % SplatSizeInBits == 0 &&
         "SplatSizeInBits must divide width!");
  // We can check that all parts of an integer are equal by making use of a
  // little trick: rotate and check if it's still the same value.
  return *this == rotl(SplatSizeInBits);
}

/// This function returns the high "numBits" bits of this APInt.
APInt APInt::getHiBits(unsigned numBits) const {
  return this->lshr(BitWidth - numBits);
}

/// This function returns the low "numBits" bits of this APInt.
APInt APInt::getLoBits(unsigned numBits) const {
  APInt Result(getLowBitsSet(BitWidth, numBits));
  Result &= *this;
  return Result;
}

unsigned APInt::countLeadingZerosSlowCase() const {
  unsigned Count = 0;
  for (int i = getNumWords()-1; i >= 0; --i) {
    uint64_t V = pVal[i];
    if (V == 0)
      Count += APINT_BITS_PER_WORD;
    else {
      Count += llvm::countLeadingZeros(V);
      break;
    }
  }
  // Adjust for unused bits in the most significant word (they are zero).
  unsigned Mod = BitWidth % APINT_BITS_PER_WORD;
  Count -= Mod > 0 ? APINT_BITS_PER_WORD - Mod : 0;
  return Count;
}

unsigned APInt::countLeadingOnes() const {
  if (isSingleWord())
    return llvm::countLeadingOnes(VAL << (APINT_BITS_PER_WORD - BitWidth));

  unsigned highWordBits = BitWidth % APINT_BITS_PER_WORD;
  unsigned shift;
  if (!highWordBits) {
    highWordBits = APINT_BITS_PER_WORD;
    shift = 0;
  } else {
    shift = APINT_BITS_PER_WORD - highWordBits;
  }
  int i = getNumWords() - 1;
  unsigned Count = llvm::countLeadingOnes(pVal[i] << shift);
  if (Count == highWordBits) {
    for (i--; i >= 0; --i) {
      if (pVal[i] == -1ULL)
        Count += APINT_BITS_PER_WORD;
      else {
        Count += llvm::countLeadingOnes(pVal[i]);
        break;
      }
    }
  }
  return Count;
}

unsigned APInt::countTrailingZeros() const {
  if (isSingleWord())
    return std::min(unsigned(llvm::countTrailingZeros(VAL)), BitWidth);
  unsigned Count = 0;
  unsigned i = 0;
  for (; i < getNumWords() && pVal[i] == 0; ++i)
    Count += APINT_BITS_PER_WORD;
  if (i < getNumWords())
    Count += llvm::countTrailingZeros(pVal[i]);
  return std::min(Count, BitWidth);
}

unsigned APInt::countTrailingOnesSlowCase() const {
  unsigned Count = 0;
  unsigned i = 0;
  for (; i < getNumWords() && pVal[i] == -1ULL; ++i)
    Count += APINT_BITS_PER_WORD;
  if (i < getNumWords())
    Count += llvm::countTrailingOnes(pVal[i]);
  return std::min(Count, BitWidth);
}

unsigned APInt::countPopulationSlowCase() const {
  unsigned Count = 0;
  for (unsigned i = 0; i < getNumWords(); ++i)
    Count += llvm::countPopulation(pVal[i]);
  return Count;
}

APInt APInt::byteSwap() const {
  assert(BitWidth >= 16 && BitWidth % 16 == 0 && "Cannot byteswap!");
  if (BitWidth == 16)
    return APInt(BitWidth, ByteSwap_16(uint16_t(VAL)));
  if (BitWidth == 32)
    return APInt(BitWidth, ByteSwap_32(unsigned(VAL)));
  if (BitWidth == 48) {
    unsigned Tmp1 = unsigned(VAL >> 16);
    Tmp1 = ByteSwap_32(Tmp1);
    uint16_t Tmp2 = uint16_t(VAL);
    Tmp2 = ByteSwap_16(Tmp2);
    return APInt(BitWidth, (uint64_t(Tmp2) << 32) | Tmp1);
  }
  if (BitWidth == 64)
    return APInt(BitWidth, ByteSwap_64(VAL));

  APInt Result(getNumWords() * APINT_BITS_PER_WORD, 0);
  for (unsigned I = 0, N = getNumWords(); I != N; ++I)
    Result.pVal[I] = ByteSwap_64(pVal[N - I - 1]);
  if (Result.BitWidth != BitWidth) {
    Result.lshrInPlace(Result.BitWidth - BitWidth);
    Result.BitWidth = BitWidth;
  }
  return Result;
}

APInt APInt::reverseBits() const {
  switch (BitWidth) {
  case 64:
    return APInt(BitWidth, llvm::reverseBits<uint64_t>(VAL));
  case 32:
    return APInt(BitWidth, llvm::reverseBits<uint32_t>(VAL));
  case 16:
    return APInt(BitWidth, llvm::reverseBits<uint16_t>(VAL));
  case 8:
    return APInt(BitWidth, llvm::reverseBits<uint8_t>(VAL));
  default:
    break;
  }

  APInt Val(*this);
  APInt Reversed(BitWidth, 0);
  unsigned S = BitWidth;

  for (; Val != 0; Val.lshrInPlace(1)) {
    Reversed <<= 1;
    Reversed |= Val[0];
    --S;
  }

  Reversed <<= S;
  return Reversed;
}

APInt llvm::APIntOps::GreatestCommonDivisor(APInt A, APInt B) {
  // Fast-path a common case.
  if (A == B) return A;

  // Corner cases: if either operand is zero, the other is the gcd.
  if (!A) return B;
  if (!B) return A;

  // Count common powers of 2 and remove all other powers of 2.
  unsigned Pow2;
  {
    unsigned Pow2_A = A.countTrailingZeros();
    unsigned Pow2_B = B.countTrailingZeros();
    if (Pow2_A > Pow2_B) {
      A.lshrInPlace(Pow2_A - Pow2_B);
      Pow2 = Pow2_B;
    } else if (Pow2_B > Pow2_A) {
      B.lshrInPlace(Pow2_B - Pow2_A);
      Pow2 = Pow2_A;
    } else {
      Pow2 = Pow2_A;
    }
  }

  // Both operands are odd multiples of 2^Pow_2:
  //
  //   gcd(a, b) = gcd(|a - b| / 2^i, min(a, b))
  //
  // This is a modified version of Stein's algorithm, taking advantage of
  // efficient countTrailingZeros().
  while (A != B) {
    if (A.ugt(B)) {
      A -= B;
      A.lshrInPlace(A.countTrailingZeros() - Pow2);
    } else {
      B -= A;
      B.lshrInPlace(B.countTrailingZeros() - Pow2);
    }
  }

  return A;
}

APInt llvm::APIntOps::RoundDoubleToAPInt(double Double, unsigned width) {
  union {
    double D;
    uint64_t I;
  } T;
  T.D = Double;

  // Get the sign bit from the highest order bit
  bool isNeg = T.I >> 63;

  // Get the 11-bit exponent and adjust for the 1023 bit bias
  int64_t exp = ((T.I >> 52) & 0x7ff) - 1023;

  // If the exponent is negative, the value is < 0 so just return 0.
  if (exp < 0)
    return APInt(width, 0u);

  // Extract the mantissa by clearing the top 12 bits (sign + exponent).
  uint64_t mantissa = (T.I & (~0ULL >> 12)) | 1ULL << 52;

  // If the exponent doesn't shift all bits out of the mantissa
  if (exp < 52)
    return isNeg ? -APInt(width, mantissa >> (52 - exp)) :
                    APInt(width, mantissa >> (52 - exp));

  // If the client didn't provide enough bits for us to shift the mantissa into
  // then the result is undefined, just return 0
  if (width <= exp - 52)
    return APInt(width, 0);

  // Otherwise, we have to shift the mantissa bits up to the right location
  APInt Tmp(width, mantissa);
  Tmp = Tmp.shl((unsigned)exp - 52);
  return isNeg ? -Tmp : Tmp;
}

/// This function converts this APInt to a double.
/// The layout for double is as following (IEEE Standard 754):
///  --------------------------------------
/// |  Sign    Exponent    Fraction    Bias |
/// |-------------------------------------- |
/// |  1[63]   11[62-52]   52[51-00]   1023 |
///  --------------------------------------
double APInt::roundToDouble(bool isSigned) const {

  // Handle the simple case where the value is contained in one uint64_t.
  // It is wrong to optimize getWord(0) to VAL; there might be more than one word.
  if (isSingleWord() || getActiveBits() <= APINT_BITS_PER_WORD) {
    if (isSigned) {
      int64_t sext = SignExtend64(getWord(0), BitWidth);
      return double(sext);
    } else
      return double(getWord(0));
  }

  // Determine if the value is negative.
  bool isNeg = isSigned ? (*this)[BitWidth-1] : false;

  // Construct the absolute value if we're negative.
  APInt Tmp(isNeg ? -(*this) : (*this));

  // Figure out how many bits we're using.
  unsigned n = Tmp.getActiveBits();

  // The exponent (without bias normalization) is just the number of bits
  // we are using. Note that the sign bit is gone since we constructed the
  // absolute value.
  uint64_t exp = n;

  // Return infinity for exponent overflow
  if (exp > 1023) {
    if (!isSigned || !isNeg)
      return std::numeric_limits<double>::infinity();
    else
      return -std::numeric_limits<double>::infinity();
  }
  exp += 1023; // Increment for 1023 bias

  // Number of bits in mantissa is 52. To obtain the mantissa value, we must
  // extract the high 52 bits from the correct words in pVal.
  uint64_t mantissa;
  unsigned hiWord = whichWord(n-1);
  if (hiWord == 0) {
    mantissa = Tmp.pVal[0];
    if (n > 52)
      mantissa >>= n - 52; // shift down, we want the top 52 bits.
  } else {
    assert(hiWord > 0 && "huh?");
    uint64_t hibits = Tmp.pVal[hiWord] << (52 - n % APINT_BITS_PER_WORD);
    uint64_t lobits = Tmp.pVal[hiWord-1] >> (11 + n % APINT_BITS_PER_WORD);
    mantissa = hibits | lobits;
  }

  // The leading bit of mantissa is implicit, so get rid of it.
  uint64_t sign = isNeg ? (1ULL << (APINT_BITS_PER_WORD - 1)) : 0;
  union {
    double D;
    uint64_t I;
  } T;
  T.I = sign | (exp << 52) | mantissa;
  return T.D;
}

// Truncate to new width.
APInt APInt::trunc(unsigned width) const {
  assert(width < BitWidth && "Invalid APInt Truncate request");
  assert(width && "Can't truncate to 0 bits");

  if (width <= APINT_BITS_PER_WORD)
    return APInt(width, getRawData()[0]);

  APInt Result(getMemory(getNumWords(width)), width);

  // Copy full words.
  unsigned i;
  for (i = 0; i != width / APINT_BITS_PER_WORD; i++)
    Result.pVal[i] = pVal[i];

  // Truncate and copy any partial word.
  unsigned bits = (0 - width) % APINT_BITS_PER_WORD;
  if (bits != 0)
    Result.pVal[i] = pVal[i] << bits >> bits;

  return Result;
}

// Sign extend to a new width.
APInt APInt::sext(unsigned width) const {
  assert(width > BitWidth && "Invalid APInt SignExtend request");

  if (width <= APINT_BITS_PER_WORD) {
    uint64_t val = VAL << (APINT_BITS_PER_WORD - BitWidth);
    val = (int64_t)val >> (width - BitWidth);
    return APInt(width, val >> (APINT_BITS_PER_WORD - width));
  }

  APInt Result(getMemory(getNumWords(width)), width);

  // Copy full words.
  unsigned i;
  uint64_t word = 0;
  for (i = 0; i != BitWidth / APINT_BITS_PER_WORD; i++) {
    word = getRawData()[i];
    Result.pVal[i] = word;
  }

  // Read and sign-extend any partial word.
  unsigned bits = (0 - BitWidth) % APINT_BITS_PER_WORD;
  if (bits != 0)
    word = (int64_t)getRawData()[i] << bits >> bits;
  else
    word = (int64_t)word >> (APINT_BITS_PER_WORD - 1);

  // Write remaining full words.
  for (; i != width / APINT_BITS_PER_WORD; i++) {
    Result.pVal[i] = word;
    word = (int64_t)word >> (APINT_BITS_PER_WORD - 1);
  }

  // Write any partial word.
  bits = (0 - width) % APINT_BITS_PER_WORD;
  if (bits != 0)
    Result.pVal[i] = word << bits >> bits;

  return Result;
}

//  Zero extend to a new width.
APInt APInt::zext(unsigned width) const {
  assert(width > BitWidth && "Invalid APInt ZeroExtend request");

  if (width <= APINT_BITS_PER_WORD)
    return APInt(width, VAL);

  APInt Result(getMemory(getNumWords(width)), width);

  // Copy words.
  unsigned i;
  for (i = 0; i != getNumWords(); i++)
    Result.pVal[i] = getRawData()[i];

  // Zero remaining words.
  memset(&Result.pVal[i], 0, (Result.getNumWords() - i) * APINT_WORD_SIZE);

  return Result;
}

APInt APInt::zextOrTrunc(unsigned width) const {
  if (BitWidth < width)
    return zext(width);
  if (BitWidth > width)
    return trunc(width);
  return *this;
}

APInt APInt::sextOrTrunc(unsigned width) const {
  if (BitWidth < width)
    return sext(width);
  if (BitWidth > width)
    return trunc(width);
  return *this;
}

APInt APInt::zextOrSelf(unsigned width) const {
  if (BitWidth < width)
    return zext(width);
  return *this;
}

APInt APInt::sextOrSelf(unsigned width) const {
  if (BitWidth < width)
    return sext(width);
  return *this;
}

/// Arithmetic right-shift this APInt by shiftAmt.
/// @brief Arithmetic right-shift function.
APInt APInt::ashr(const APInt &shiftAmt) const {
  return ashr((unsigned)shiftAmt.getLimitedValue(BitWidth));
}

/// Arithmetic right-shift this APInt by shiftAmt.
/// @brief Arithmetic right-shift function.
APInt APInt::ashr(unsigned shiftAmt) const {
  assert(shiftAmt <= BitWidth && "Invalid shift amount");
  // Handle a degenerate case
  if (shiftAmt == 0)
    return *this;

  // Handle single word shifts with built-in ashr
  if (isSingleWord()) {
    if (shiftAmt == BitWidth)
      return APInt(BitWidth, 0); // undefined
    return APInt(BitWidth, SignExtend64(VAL, BitWidth) >> shiftAmt);
  }

  // If all the bits were shifted out, the result is, technically, undefined.
  // We return -1 if it was negative, 0 otherwise. We check this early to avoid
  // issues in the algorithm below.
  if (shiftAmt == BitWidth) {
    if (isNegative())
      return APInt(BitWidth, -1ULL, true);
    else
      return APInt(BitWidth, 0);
  }

  // Create some space for the result.
  uint64_t * val = new uint64_t[getNumWords()];

  // Compute some values needed by the following shift algorithms
  unsigned wordShift = shiftAmt % APINT_BITS_PER_WORD; // bits to shift per word
  unsigned offset = shiftAmt / APINT_BITS_PER_WORD; // word offset for shift
  unsigned breakWord = getNumWords() - 1 - offset; // last word affected
  unsigned bitsInWord = whichBit(BitWidth); // how many bits in last word?
  if (bitsInWord == 0)
    bitsInWord = APINT_BITS_PER_WORD;

  // If we are shifting whole words, just move whole words
  if (wordShift == 0) {
    // Move the words containing significant bits
    for (unsigned i = 0; i <= breakWord; ++i)
      val[i] = pVal[i+offset]; // move whole word

    // Adjust the top significant word for sign bit fill, if negative
    if (isNegative())
      if (bitsInWord < APINT_BITS_PER_WORD)
        val[breakWord] |= ~0ULL << bitsInWord; // set high bits
  } else {
    // Shift the low order words
    for (unsigned i = 0; i < breakWord; ++i) {
      // This combines the shifted corresponding word with the low bits from
      // the next word (shifted into this word's high bits).
      val[i] = (pVal[i+offset] >> wordShift) |
               (pVal[i+offset+1] << (APINT_BITS_PER_WORD - wordShift));
    }

    // Shift the break word. In this case there are no bits from the next word
    // to include in this word.
    val[breakWord] = pVal[breakWord+offset] >> wordShift;

    // Deal with sign extension in the break word, and possibly the word before
    // it.
    if (isNegative()) {
      if (wordShift > bitsInWord) {
        if (breakWord > 0)
          val[breakWord-1] |=
            ~0ULL << (APINT_BITS_PER_WORD - (wordShift - bitsInWord));
        val[breakWord] |= ~0ULL;
      } else
        val[breakWord] |= (~0ULL << (bitsInWord - wordShift));
    }
  }

  // Remaining words are 0 or -1, just assign them.
  uint64_t fillValue = (isNegative() ? -1ULL : 0);
  for (unsigned i = breakWord+1; i < getNumWords(); ++i)
    val[i] = fillValue;
  APInt Result(val, BitWidth);
  Result.clearUnusedBits();
  return Result;
}

/// Logical right-shift this APInt by shiftAmt.
/// @brief Logical right-shift function.
void APInt::lshrInPlace(const APInt &shiftAmt) {
  lshrInPlace((unsigned)shiftAmt.getLimitedValue(BitWidth));
}

/// Logical right-shift this APInt by shiftAmt.
/// @brief Logical right-shift function.
void APInt::lshrSlowCase(unsigned ShiftAmt) {
  tcShiftRight(pVal, getNumWords(), ShiftAmt);
}

/// Left-shift this APInt by shiftAmt.
/// @brief Left-shift function.
APInt APInt::shl(const APInt &shiftAmt) const {
  // It's undefined behavior in C to shift by BitWidth or greater.
  return shl((unsigned)shiftAmt.getLimitedValue(BitWidth));
}

void APInt::shlSlowCase(unsigned ShiftAmt) {
  tcShiftLeft(pVal, getNumWords(), ShiftAmt);
  clearUnusedBits();
}

// Calculate the rotate amount modulo the bit width.
static unsigned rotateModulo(unsigned BitWidth, const APInt &rotateAmt) {
  unsigned rotBitWidth = rotateAmt.getBitWidth();
  APInt rot = rotateAmt;
  if (rotBitWidth < BitWidth) {
    // Extend the rotate APInt, so that the urem doesn't divide by 0.
    // e.g. APInt(1, 32) would give APInt(1, 0).
    rot = rotateAmt.zext(BitWidth);
  }
  rot = rot.urem(APInt(rot.getBitWidth(), BitWidth));
  return rot.getLimitedValue(BitWidth);
}

APInt APInt::rotl(const APInt &rotateAmt) const {
  return rotl(rotateModulo(BitWidth, rotateAmt));
}

APInt APInt::rotl(unsigned rotateAmt) const {
  rotateAmt %= BitWidth;
  if (rotateAmt == 0)
    return *this;
  return shl(rotateAmt) | lshr(BitWidth - rotateAmt);
}

APInt APInt::rotr(const APInt &rotateAmt) const {
  return rotr(rotateModulo(BitWidth, rotateAmt));
}

APInt APInt::rotr(unsigned rotateAmt) const {
  rotateAmt %= BitWidth;
  if (rotateAmt == 0)
    return *this;
  return lshr(rotateAmt) | shl(BitWidth - rotateAmt);
}

// Square Root - this method computes and returns the square root of "this".
// Three mechanisms are used for computation. For small values (<= 5 bits),
// a table lookup is done. This gets some performance for common cases. For
// values using less than 52 bits, the value is converted to double and then
// the libc sqrt function is called. The result is rounded and then converted
// back to a uint64_t which is then used to construct the result. Finally,
// the Babylonian method for computing square roots is used.
APInt APInt::sqrt() const {

  // Determine the magnitude of the value.
  unsigned magnitude = getActiveBits();

  // Use a fast table for some small values. This also gets rid of some
  // rounding errors in libc sqrt for small values.
  if (magnitude <= 5) {
    static const uint8_t results[32] = {
      /*     0 */ 0,
      /*  1- 2 */ 1, 1,
      /*  3- 6 */ 2, 2, 2, 2,
      /*  7-12 */ 3, 3, 3, 3, 3, 3,
      /* 13-20 */ 4, 4, 4, 4, 4, 4, 4, 4,
      /* 21-30 */ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      /*    31 */ 6
    };
    return APInt(BitWidth, results[ (isSingleWord() ? VAL : pVal[0]) ]);
  }

  // If the magnitude of the value fits in less than 52 bits (the precision of
  // an IEEE double precision floating point value), then we can use the
  // libc sqrt function which will probably use a hardware sqrt computation.
  // This should be faster than the algorithm below.
  if (magnitude < 52) {
    return APInt(BitWidth,
                 uint64_t(::round(::sqrt(double(isSingleWord()?VAL:pVal[0])))));
  }

  // Okay, all the short cuts are exhausted. We must compute it. The following
  // is a classical Babylonian method for computing the square root. This code
  // was adapted to APInt from a wikipedia article on such computations.
  // See http://www.wikipedia.org/ and go to the page named
  // Calculate_an_integer_square_root.
  unsigned nbits = BitWidth, i = 4;
  APInt testy(BitWidth, 16);
  APInt x_old(BitWidth, 1);
  APInt x_new(BitWidth, 0);
  APInt two(BitWidth, 2);

  // Select a good starting value using binary logarithms.
  for (;; i += 2, testy = testy.shl(2))
    if (i >= nbits || this->ule(testy)) {
      x_old = x_old.shl(i / 2);
      break;
    }

  // Use the Babylonian method to arrive at the integer square root:
  for (;;) {
    x_new = (this->udiv(x_old) + x_old).udiv(two);
    if (x_old.ule(x_new))
      break;
    x_old = x_new;
  }

  // Make sure we return the closest approximation
  // NOTE: The rounding calculation below is correct. It will produce an
  // off-by-one discrepancy with results from pari/gp. That discrepancy has been
  // determined to be a rounding issue with pari/gp as it begins to use a
  // floating point representation after 192 bits. There are no discrepancies
  // between this algorithm and pari/gp for bit widths < 192 bits.
  APInt square(x_old * x_old);
  APInt nextSquare((x_old + 1) * (x_old +1));
  if (this->ult(square))
    return x_old;
  assert(this->ule(nextSquare) && "Error in APInt::sqrt computation");
  APInt midpoint((nextSquare - square).udiv(two));
  APInt offset(*this - square);
  if (offset.ult(midpoint))
    return x_old;
  return x_old + 1;
}

/// Computes the multiplicative inverse of this APInt for a given modulo. The
/// iterative extended Euclidean algorithm is used to solve for this value,
/// however we simplify it to speed up calculating only the inverse, and take
/// advantage of div+rem calculations. We also use some tricks to avoid copying
/// (potentially large) APInts around.
APInt APInt::multiplicativeInverse(const APInt& modulo) const {
  assert(ult(modulo) && "This APInt must be smaller than the modulo");

  // Using the properties listed at the following web page (accessed 06/21/08):
  //   http://www.numbertheory.org/php/euclid.html
  // (especially the properties numbered 3, 4 and 9) it can be proved that
  // BitWidth bits suffice for all the computations in the algorithm implemented
  // below. More precisely, this number of bits suffice if the multiplicative
  // inverse exists, but may not suffice for the general extended Euclidean
  // algorithm.

  APInt r[2] = { modulo, *this };
  APInt t[2] = { APInt(BitWidth, 0), APInt(BitWidth, 1) };
  APInt q(BitWidth, 0);

  unsigned i;
  for (i = 0; r[i^1] != 0; i ^= 1) {
    // An overview of the math without the confusing bit-flipping:
    // q = r[i-2] / r[i-1]
    // r[i] = r[i-2] % r[i-1]
    // t[i] = t[i-2] - t[i-1] * q
    udivrem(r[i], r[i^1], q, r[i]);
    t[i] -= t[i^1] * q;
  }

  // If this APInt and the modulo are not coprime, there is no multiplicative
  // inverse, so return 0. We check this by looking at the next-to-last
  // remainder, which is the gcd(*this,modulo) as calculated by the Euclidean
  // algorithm.
  if (r[i] != 1)
    return APInt(BitWidth, 0);

  // The next-to-last t is the multiplicative inverse.  However, we are
  // interested in a positive inverse. Calcuate a positive one from a negative
  // one if necessary. A simple addition of the modulo suffices because
  // abs(t[i]) is known to be less than *this/2 (see the link above).
  return t[i].isNegative() ? t[i] + modulo : t[i];
}

/// Calculate the magic numbers required to implement a signed integer division
/// by a constant as a sequence of multiplies, adds and shifts.  Requires that
/// the divisor not be 0, 1, or -1.  Taken from "Hacker's Delight", Henry S.
/// Warren, Jr., chapter 10.
APInt::ms APInt::magic() const {
  const APInt& d = *this;
  unsigned p;
  APInt ad, anc, delta, q1, r1, q2, r2, t;
  APInt signedMin = APInt::getSignedMinValue(d.getBitWidth());
  struct ms mag;

  ad = d.abs();
  t = signedMin + (d.lshr(d.getBitWidth() - 1));
  anc = t - 1 - t.urem(ad);   // absolute value of nc
  p = d.getBitWidth() - 1;    // initialize p
  q1 = signedMin.udiv(anc);   // initialize q1 = 2p/abs(nc)
  r1 = signedMin - q1*anc;    // initialize r1 = rem(2p,abs(nc))
  q2 = signedMin.udiv(ad);    // initialize q2 = 2p/abs(d)
  r2 = signedMin - q2*ad;     // initialize r2 = rem(2p,abs(d))
  do {
    p = p + 1;
    q1 = q1<<1;          // update q1 = 2p/abs(nc)
    r1 = r1<<1;          // update r1 = rem(2p/abs(nc))
    if (r1.uge(anc)) {  // must be unsigned comparison
      q1 = q1 + 1;
      r1 = r1 - anc;
    }
    q2 = q2<<1;          // update q2 = 2p/abs(d)
    r2 = r2<<1;          // update r2 = rem(2p/abs(d))
    if (r2.uge(ad)) {   // must be unsigned comparison
      q2 = q2 + 1;
      r2 = r2 - ad;
    }
    delta = ad - r2;
  } while (q1.ult(delta) || (q1 == delta && r1 == 0));

  mag.m = q2 + 1;
  if (d.isNegative()) mag.m = -mag.m;   // resulting magic number
  mag.s = p - d.getBitWidth();          // resulting shift
  return mag;
}

/// Calculate the magic numbers required to implement an unsigned integer
/// division by a constant as a sequence of multiplies, adds and shifts.
/// Requires that the divisor not be 0.  Taken from "Hacker's Delight", Henry
/// S. Warren, Jr., chapter 10.
/// LeadingZeros can be used to simplify the calculation if the upper bits
/// of the divided value are known zero.
APInt::mu APInt::magicu(unsigned LeadingZeros) const {
  const APInt& d = *this;
  unsigned p;
  APInt nc, delta, q1, r1, q2, r2;
  struct mu magu;
  magu.a = 0;               // initialize "add" indicator
  APInt allOnes = APInt::getAllOnesValue(d.getBitWidth()).lshr(LeadingZeros);
  APInt signedMin = APInt::getSignedMinValue(d.getBitWidth());
  APInt signedMax = APInt::getSignedMaxValue(d.getBitWidth());

  nc = allOnes - (allOnes - d).urem(d);
  p = d.getBitWidth() - 1;  // initialize p
  q1 = signedMin.udiv(nc);  // initialize q1 = 2p/nc
  r1 = signedMin - q1*nc;   // initialize r1 = rem(2p,nc)
  q2 = signedMax.udiv(d);   // initialize q2 = (2p-1)/d
  r2 = signedMax - q2*d;    // initialize r2 = rem((2p-1),d)
  do {
    p = p + 1;
    if (r1.uge(nc - r1)) {
      q1 = q1 + q1 + 1;  // update q1
      r1 = r1 + r1 - nc; // update r1
    }
    else {
      q1 = q1+q1; // update q1
      r1 = r1+r1; // update r1
    }
    if ((r2 + 1).uge(d - r2)) {
      if (q2.uge(signedMax)) magu.a = 1;
      q2 = q2+q2 + 1;     // update q2
      r2 = r2+r2 + 1 - d; // update r2
    }
    else {
      if (q2.uge(signedMin)) magu.a = 1;
      q2 = q2+q2;     // update q2
      r2 = r2+r2 + 1; // update r2
    }
    delta = d - 1 - r2;
  } while (p < d.getBitWidth()*2 &&
           (q1.ult(delta) || (q1 == delta && r1 == 0)));
  magu.m = q2 + 1; // resulting magic number
  magu.s = p - d.getBitWidth();  // resulting shift
  return magu;
}

/// Implementation of Knuth's Algorithm D (Division of nonnegative integers)
/// from "Art of Computer Programming, Volume 2", section 4.3.1, p. 272. The
/// variables here have the same names as in the algorithm. Comments explain
/// the algorithm and any deviation from it.
static void KnuthDiv(unsigned *u, unsigned *v, unsigned *q, unsigned* r,
                     unsigned m, unsigned n) {
  assert(u && "Must provide dividend");
  assert(v && "Must provide divisor");
  assert(q && "Must provide quotient");
  assert(u != v && u != q && v != q && "Must use different memory");
  assert(n>1 && "n must be > 1");

  // b denotes the base of the number system. In our case b is 2^32.
  const uint64_t b = uint64_t(1) << 32;

  DEBUG(dbgs() << "KnuthDiv: m=" << m << " n=" << n << '\n');
  DEBUG(dbgs() << "KnuthDiv: original:");
  DEBUG(for (int i = m+n; i >=0; i--) dbgs() << " " << u[i]);
  DEBUG(dbgs() << " by");
  DEBUG(for (int i = n; i >0; i--) dbgs() << " " << v[i-1]);
  DEBUG(dbgs() << '\n');
  // D1. [Normalize.] Set d = b / (v[n-1] + 1) and multiply all the digits of
  // u and v by d. Note that we have taken Knuth's advice here to use a power
  // of 2 value for d such that d * v[n-1] >= b/2 (b is the base). A power of
  // 2 allows us to shift instead of multiply and it is easy to determine the
  // shift amount from the leading zeros.  We are basically normalizing the u
  // and v so that its high bits are shifted to the top of v's range without
  // overflow. Note that this can require an extra word in u so that u must
  // be of length m+n+1.
  unsigned shift = countLeadingZeros(v[n-1]);
  unsigned v_carry = 0;
  unsigned u_carry = 0;
  if (shift) {
    for (unsigned i = 0; i < m+n; ++i) {
      unsigned u_tmp = u[i] >> (32 - shift);
      u[i] = (u[i] << shift) | u_carry;
      u_carry = u_tmp;
    }
    for (unsigned i = 0; i < n; ++i) {
      unsigned v_tmp = v[i] >> (32 - shift);
      v[i] = (v[i] << shift) | v_carry;
      v_carry = v_tmp;
    }
  }
  u[m+n] = u_carry;

  DEBUG(dbgs() << "KnuthDiv:   normal:");
  DEBUG(for (int i = m+n; i >=0; i--) dbgs() << " " << u[i]);
  DEBUG(dbgs() << " by");
  DEBUG(for (int i = n; i >0; i--) dbgs() << " " << v[i-1]);
  DEBUG(dbgs() << '\n');

  // D2. [Initialize j.]  Set j to m. This is the loop counter over the places.
  int j = m;
  do {
    DEBUG(dbgs() << "KnuthDiv: quotient digit #" << j << '\n');
    // D3. [Calculate q'.].
    //     Set qp = (u[j+n]*b + u[j+n-1]) / v[n-1]. (qp=qprime=q')
    //     Set rp = (u[j+n]*b + u[j+n-1]) % v[n-1]. (rp=rprime=r')
    // Now test if qp == b or qp*v[n-2] > b*rp + u[j+n-2]; if so, decrease
    // qp by 1, inrease rp by v[n-1], and repeat this test if rp < b. The test
    // on v[n-2] determines at high speed most of the cases in which the trial
    // value qp is one too large, and it eliminates all cases where qp is two
    // too large.
    uint64_t dividend = ((uint64_t(u[j+n]) << 32) + u[j+n-1]);
    DEBUG(dbgs() << "KnuthDiv: dividend == " << dividend << '\n');
    uint64_t qp = dividend / v[n-1];
    uint64_t rp = dividend % v[n-1];
    if (qp == b || qp*v[n-2] > b*rp + u[j+n-2]) {
      qp--;
      rp += v[n-1];
      if (rp < b && (qp == b || qp*v[n-2] > b*rp + u[j+n-2]))
        qp--;
    }
    DEBUG(dbgs() << "KnuthDiv: qp == " << qp << ", rp == " << rp << '\n');

    // D4. [Multiply and subtract.] Replace (u[j+n]u[j+n-1]...u[j]) with
    // (u[j+n]u[j+n-1]..u[j]) - qp * (v[n-1]...v[1]v[0]). This computation
    // consists of a simple multiplication by a one-place number, combined with
    // a subtraction.
    // The digits (u[j+n]...u[j]) should be kept positive; if the result of
    // this step is actually negative, (u[j+n]...u[j]) should be left as the
    // true value plus b**(n+1), namely as the b's complement of
    // the true value, and a "borrow" to the left should be remembered.
    int64_t borrow = 0;
    for (unsigned i = 0; i < n; ++i) {
      uint64_t p = uint64_t(qp) * uint64_t(v[i]);
      int64_t subres = int64_t(u[j+i]) - borrow - (unsigned)p;
      u[j+i] = (unsigned)subres;
      borrow = (p >> 32) - (subres >> 32);
      DEBUG(dbgs() << "KnuthDiv: u[j+i] = " << u[j+i]
                   << ", borrow = " << borrow << '\n');
    }
    bool isNeg = u[j+n] < borrow;
    u[j+n] -= (unsigned)borrow;

    DEBUG(dbgs() << "KnuthDiv: after subtraction:");
    DEBUG(for (int i = m+n; i >=0; i--) dbgs() << " " << u[i]);
    DEBUG(dbgs() << '\n');

    // D5. [Test remainder.] Set q[j] = qp. If the result of step D4 was
    // negative, go to step D6; otherwise go on to step D7.
    q[j] = (unsigned)qp;
    if (isNeg) {
      // D6. [Add back]. The probability that this step is necessary is very
      // small, on the order of only 2/b. Make sure that test data accounts for
      // this possibility. Decrease q[j] by 1
      q[j]--;
      // and add (0v[n-1]...v[1]v[0]) to (u[j+n]u[j+n-1]...u[j+1]u[j]).
      // A carry will occur to the left of u[j+n], and it should be ignored
      // since it cancels with the borrow that occurred in D4.
      bool carry = false;
      for (unsigned i = 0; i < n; i++) {
        unsigned limit = std::min(u[j+i],v[i]);
        u[j+i] += v[i] + carry;
        carry = u[j+i] < limit || (carry && u[j+i] == limit);
      }
      u[j+n] += carry;
    }
    DEBUG(dbgs() << "KnuthDiv: after correction:");
    DEBUG(for (int i = m+n; i >=0; i--) dbgs() << " " << u[i]);
    DEBUG(dbgs() << "\nKnuthDiv: digit result = " << q[j] << '\n');

  // D7. [Loop on j.]  Decrease j by one. Now if j >= 0, go back to D3.
  } while (--j >= 0);

  DEBUG(dbgs() << "KnuthDiv: quotient:");
  DEBUG(for (int i = m; i >=0; i--) dbgs() <<" " << q[i]);
  DEBUG(dbgs() << '\n');

  // D8. [Unnormalize]. Now q[...] is the desired quotient, and the desired
  // remainder may be obtained by dividing u[...] by d. If r is non-null we
  // compute the remainder (urem uses this).
  if (r) {
    // The value d is expressed by the "shift" value above since we avoided
    // multiplication by d by using a shift left. So, all we have to do is
    // shift right here.
    if (shift) {
      unsigned carry = 0;
      DEBUG(dbgs() << "KnuthDiv: remainder:");
      for (int i = n-1; i >= 0; i--) {
        r[i] = (u[i] >> shift) | carry;
        carry = u[i] << (32 - shift);
        DEBUG(dbgs() << " " << r[i]);
      }
    } else {
      for (int i = n-1; i >= 0; i--) {
        r[i] = u[i];
        DEBUG(dbgs() << " " << r[i]);
      }
    }
    DEBUG(dbgs() << '\n');
  }
  DEBUG(dbgs() << '\n');
}

void APInt::divide(const APInt &LHS, unsigned lhsWords, const APInt &RHS,
                   unsigned rhsWords, APInt *Quotient, APInt *Remainder) {
  assert(lhsWords >= rhsWords && "Fractional result");

  // First, compose the values into an array of 32-bit words instead of
  // 64-bit words. This is a necessity of both the "short division" algorithm
  // and the Knuth "classical algorithm" which requires there to be native
  // operations for +, -, and * on an m bit value with an m*2 bit result. We
  // can't use 64-bit operands here because we don't have native results of
  // 128-bits. Furthermore, casting the 64-bit values to 32-bit values won't
  // work on large-endian machines.
  uint64_t mask = ~0ull >> (sizeof(unsigned)*CHAR_BIT);
  unsigned n = rhsWords * 2;
  unsigned m = (lhsWords * 2) - n;

  // Allocate space for the temporary values we need either on the stack, if
  // it will fit, or on the heap if it won't.
  unsigned SPACE[128];
  unsigned *U = nullptr;
  unsigned *V = nullptr;
  unsigned *Q = nullptr;
  unsigned *R = nullptr;
  if ((Remainder?4:3)*n+2*m+1 <= 128) {
    U = &SPACE[0];
    V = &SPACE[m+n+1];
    Q = &SPACE[(m+n+1) + n];
    if (Remainder)
      R = &SPACE[(m+n+1) + n + (m+n)];
  } else {
    U = new unsigned[m + n + 1];
    V = new unsigned[n];
    Q = new unsigned[m+n];
    if (Remainder)
      R = new unsigned[n];
  }

  // Initialize the dividend
  memset(U, 0, (m+n+1)*sizeof(unsigned));
  for (unsigned i = 0; i < lhsWords; ++i) {
    uint64_t tmp = (LHS.getNumWords() == 1 ? LHS.VAL : LHS.pVal[i]);
    U[i * 2] = (unsigned)(tmp & mask);
    U[i * 2 + 1] = (unsigned)(tmp >> (sizeof(unsigned)*CHAR_BIT));
  }
  U[m+n] = 0; // this extra word is for "spill" in the Knuth algorithm.

  // Initialize the divisor
  memset(V, 0, (n)*sizeof(unsigned));
  for (unsigned i = 0; i < rhsWords; ++i) {
    uint64_t tmp = (RHS.getNumWords() == 1 ? RHS.VAL : RHS.pVal[i]);
    V[i * 2] = (unsigned)(tmp & mask);
    V[i * 2 + 1] = (unsigned)(tmp >> (sizeof(unsigned)*CHAR_BIT));
  }

  // initialize the quotient and remainder
  memset(Q, 0, (m+n) * sizeof(unsigned));
  if (Remainder)
    memset(R, 0, n * sizeof(unsigned));

  // Now, adjust m and n for the Knuth division. n is the number of words in
  // the divisor. m is the number of words by which the dividend exceeds the
  // divisor (i.e. m+n is the length of the dividend). These sizes must not
  // contain any zero words or the Knuth algorithm fails.
  for (unsigned i = n; i > 0 && V[i-1] == 0; i--) {
    n--;
    m++;
  }
  for (unsigned i = m+n; i > 0 && U[i-1] == 0; i--)
    m--;

  // If we're left with only a single word for the divisor, Knuth doesn't work
  // so we implement the short division algorithm here. This is much simpler
  // and faster because we are certain that we can divide a 64-bit quantity
  // by a 32-bit quantity at hardware speed and short division is simply a
  // series of such operations. This is just like doing short division but we
  // are using base 2^32 instead of base 10.
  assert(n != 0 && "Divide by zero?");
  if (n == 1) {
    unsigned divisor = V[0];
    unsigned remainder = 0;
    for (int i = m+n-1; i >= 0; i--) {
      uint64_t partial_dividend = uint64_t(remainder) << 32 | U[i];
      if (partial_dividend == 0) {
        Q[i] = 0;
        remainder = 0;
      } else if (partial_dividend < divisor) {
        Q[i] = 0;
        remainder = (unsigned)partial_dividend;
      } else if (partial_dividend == divisor) {
        Q[i] = 1;
        remainder = 0;
      } else {
        Q[i] = (unsigned)(partial_dividend / divisor);
        remainder = (unsigned)(partial_dividend - (Q[i] * divisor));
      }
    }
    if (R)
      R[0] = remainder;
  } else {
    // Now we're ready to invoke the Knuth classical divide algorithm. In this
    // case n > 1.
    KnuthDiv(U, V, Q, R, m, n);
  }

  // If the caller wants the quotient
  if (Quotient) {
    // Set up the Quotient value's memory.
    if (Quotient->BitWidth != LHS.BitWidth) {
      if (Quotient->isSingleWord())
        Quotient->VAL = 0;
      else
        delete [] Quotient->pVal;
      Quotient->BitWidth = LHS.BitWidth;
      if (!Quotient->isSingleWord())
        Quotient->pVal = getClearedMemory(Quotient->getNumWords());
    } else
      Quotient->clearAllBits();

    // The quotient is in Q. Reconstitute the quotient into Quotient's low
    // order words.
    // This case is currently dead as all users of divide() handle trivial cases
    // earlier.
    if (lhsWords == 1) {
      uint64_t tmp =
        uint64_t(Q[0]) | (uint64_t(Q[1]) << (APINT_BITS_PER_WORD / 2));
      if (Quotient->isSingleWord())
        Quotient->VAL = tmp;
      else
        Quotient->pVal[0] = tmp;
    } else {
      assert(!Quotient->isSingleWord() && "Quotient APInt not large enough");
      for (unsigned i = 0; i < lhsWords; ++i)
        Quotient->pVal[i] =
          uint64_t(Q[i*2]) | (uint64_t(Q[i*2+1]) << (APINT_BITS_PER_WORD / 2));
    }
  }

  // If the caller wants the remainder
  if (Remainder) {
    // Set up the Remainder value's memory.
    if (Remainder->BitWidth != RHS.BitWidth) {
      if (Remainder->isSingleWord())
        Remainder->VAL = 0;
      else
        delete [] Remainder->pVal;
      Remainder->BitWidth = RHS.BitWidth;
      if (!Remainder->isSingleWord())
        Remainder->pVal = getClearedMemory(Remainder->getNumWords());
    } else
      Remainder->clearAllBits();

    // The remainder is in R. Reconstitute the remainder into Remainder's low
    // order words.
    if (rhsWords == 1) {
      uint64_t tmp =
        uint64_t(R[0]) | (uint64_t(R[1]) << (APINT_BITS_PER_WORD / 2));
      if (Remainder->isSingleWord())
        Remainder->VAL = tmp;
      else
        Remainder->pVal[0] = tmp;
    } else {
      assert(!Remainder->isSingleWord() && "Remainder APInt not large enough");
      for (unsigned i = 0; i < rhsWords; ++i)
        Remainder->pVal[i] =
          uint64_t(R[i*2]) | (uint64_t(R[i*2+1]) << (APINT_BITS_PER_WORD / 2));
    }
  }

  // Clean up the memory we allocated.
  if (U != &SPACE[0]) {
    delete [] U;
    delete [] V;
    delete [] Q;
    delete [] R;
  }
}

APInt APInt::udiv(const APInt& RHS) const {
  assert(BitWidth == RHS.BitWidth && "Bit widths must be the same");

  // First, deal with the easy case
  if (isSingleWord()) {
    assert(RHS.VAL != 0 && "Divide by zero?");
    return APInt(BitWidth, VAL / RHS.VAL);
  }

  // Get some facts about the LHS and RHS number of bits and words
  unsigned rhsBits = RHS.getActiveBits();
  unsigned rhsWords = !rhsBits ? 0 : (APInt::whichWord(rhsBits - 1) + 1);
  assert(rhsWords && "Divided by zero???");
  unsigned lhsBits = this->getActiveBits();
  unsigned lhsWords = !lhsBits ? 0 : (APInt::whichWord(lhsBits - 1) + 1);

  // Deal with some degenerate cases
  if (!lhsWords)
    // 0 / X ===> 0
    return APInt(BitWidth, 0);
  else if (lhsWords < rhsWords || this->ult(RHS)) {
    // X / Y ===> 0, iff X < Y
    return APInt(BitWidth, 0);
  } else if (*this == RHS) {
    // X / X ===> 1
    return APInt(BitWidth, 1);
  } else if (lhsWords == 1 && rhsWords == 1) {
    // All high words are zero, just use native divide
    return APInt(BitWidth, this->pVal[0] / RHS.pVal[0]);
  }

  // We have to compute it the hard way. Invoke the Knuth divide algorithm.
  APInt Quotient(1,0); // to hold result.
  divide(*this, lhsWords, RHS, rhsWords, &Quotient, nullptr);
  return Quotient;
}

APInt APInt::sdiv(const APInt &RHS) const {
  if (isNegative()) {
    if (RHS.isNegative())
      return (-(*this)).udiv(-RHS);
    return -((-(*this)).udiv(RHS));
  }
  if (RHS.isNegative())
    return -(this->udiv(-RHS));
  return this->udiv(RHS);
}

APInt APInt::urem(const APInt& RHS) const {
  assert(BitWidth == RHS.BitWidth && "Bit widths must be the same");
  if (isSingleWord()) {
    assert(RHS.VAL != 0 && "Remainder by zero?");
    return APInt(BitWidth, VAL % RHS.VAL);
  }

  // Get some facts about the LHS
  unsigned lhsBits = getActiveBits();
  unsigned lhsWords = !lhsBits ? 0 : (whichWord(lhsBits - 1) + 1);

  // Get some facts about the RHS
  unsigned rhsBits = RHS.getActiveBits();
  unsigned rhsWords = !rhsBits ? 0 : (APInt::whichWord(rhsBits - 1) + 1);
  assert(rhsWords && "Performing remainder operation by zero ???");

  // Check the degenerate cases
  if (lhsWords == 0) {
    // 0 % Y ===> 0
    return APInt(BitWidth, 0);
  } else if (lhsWords < rhsWords || this->ult(RHS)) {
    // X % Y ===> X, iff X < Y
    return *this;
  } else if (*this == RHS) {
    // X % X == 0;
    return APInt(BitWidth, 0);
  } else if (lhsWords == 1) {
    // All high words are zero, just use native remainder
    return APInt(BitWidth, pVal[0] % RHS.pVal[0]);
  }

  // We have to compute it the hard way. Invoke the Knuth divide algorithm.
  APInt Remainder(1,0);
  divide(*this, lhsWords, RHS, rhsWords, nullptr, &Remainder);
  return Remainder;
}

APInt APInt::srem(const APInt &RHS) const {
  if (isNegative()) {
    if (RHS.isNegative())
      return -((-(*this)).urem(-RHS));
    return -((-(*this)).urem(RHS));
  }
  if (RHS.isNegative())
    return this->urem(-RHS);
  return this->urem(RHS);
}

void APInt::udivrem(const APInt &LHS, const APInt &RHS,
                    APInt &Quotient, APInt &Remainder) {
  assert(LHS.BitWidth == RHS.BitWidth && "Bit widths must be the same");

  // First, deal with the easy case
  if (LHS.isSingleWord()) {
    assert(RHS.VAL != 0 && "Divide by zero?");
    uint64_t QuotVal = LHS.VAL / RHS.VAL;
    uint64_t RemVal = LHS.VAL % RHS.VAL;
    Quotient = APInt(LHS.BitWidth, QuotVal);
    Remainder = APInt(LHS.BitWidth, RemVal);
    return;
  }

  // Get some size facts about the dividend and divisor
  unsigned lhsBits  = LHS.getActiveBits();
  unsigned lhsWords = !lhsBits ? 0 : (APInt::whichWord(lhsBits - 1) + 1);
  unsigned rhsBits  = RHS.getActiveBits();
  unsigned rhsWords = !rhsBits ? 0 : (APInt::whichWord(rhsBits - 1) + 1);

  // Check the degenerate cases
  if (lhsWords == 0) {
    Quotient = 0;                // 0 / Y ===> 0
    Remainder = 0;               // 0 % Y ===> 0
    return;
  }

  if (lhsWords < rhsWords || LHS.ult(RHS)) {
    Remainder = LHS;            // X % Y ===> X, iff X < Y
    Quotient = 0;               // X / Y ===> 0, iff X < Y
    return;
  }

  if (LHS == RHS) {
    Quotient  = 1;              // X / X ===> 1
    Remainder = 0;              // X % X ===> 0;
    return;
  }

  if (lhsWords == 1 && rhsWords == 1) {
    // There is only one word to consider so use the native versions.
    uint64_t lhsValue = LHS.isSingleWord() ? LHS.VAL : LHS.pVal[0];
    uint64_t rhsValue = RHS.isSingleWord() ? RHS.VAL : RHS.pVal[0];
    Quotient = APInt(LHS.getBitWidth(), lhsValue / rhsValue);
    Remainder = APInt(LHS.getBitWidth(), lhsValue % rhsValue);
    return;
  }

  // Okay, lets do it the long way
  divide(LHS, lhsWords, RHS, rhsWords, &Quotient, &Remainder);
}

void APInt::sdivrem(const APInt &LHS, const APInt &RHS,
                    APInt &Quotient, APInt &Remainder) {
  if (LHS.isNegative()) {
    if (RHS.isNegative())
      APInt::udivrem(-LHS, -RHS, Quotient, Remainder);
    else {
      APInt::udivrem(-LHS, RHS, Quotient, Remainder);
      Quotient = -Quotient;
    }
    Remainder = -Remainder;
  } else if (RHS.isNegative()) {
    APInt::udivrem(LHS, -RHS, Quotient, Remainder);
    Quotient = -Quotient;
  } else {
    APInt::udivrem(LHS, RHS, Quotient, Remainder);
  }
}

APInt APInt::sadd_ov(const APInt &RHS, bool &Overflow) const {
  APInt Res = *this+RHS;
  Overflow = isNonNegative() == RHS.isNonNegative() &&
             Res.isNonNegative() != isNonNegative();
  return Res;
}

APInt APInt::uadd_ov(const APInt &RHS, bool &Overflow) const {
  APInt Res = *this+RHS;
  Overflow = Res.ult(RHS);
  return Res;
}

APInt APInt::ssub_ov(const APInt &RHS, bool &Overflow) const {
  APInt Res = *this - RHS;
  Overflow = isNonNegative() != RHS.isNonNegative() &&
             Res.isNonNegative() != isNonNegative();
  return Res;
}

APInt APInt::usub_ov(const APInt &RHS, bool &Overflow) const {
  APInt Res = *this-RHS;
  Overflow = Res.ugt(*this);
  return Res;
}

APInt APInt::sdiv_ov(const APInt &RHS, bool &Overflow) const {
  // MININT/-1  -->  overflow.
  Overflow = isMinSignedValue() && RHS.isAllOnesValue();
  return sdiv(RHS);
}

APInt APInt::smul_ov(const APInt &RHS, bool &Overflow) const {
  APInt Res = *this * RHS;

  if (*this != 0 && RHS != 0)
    Overflow = Res.sdiv(RHS) != *this || Res.sdiv(*this) != RHS;
  else
    Overflow = false;
  return Res;
}

APInt APInt::umul_ov(const APInt &RHS, bool &Overflow) const {
  APInt Res = *this * RHS;

  if (*this != 0 && RHS != 0)
    Overflow = Res.udiv(RHS) != *this || Res.udiv(*this) != RHS;
  else
    Overflow = false;
  return Res;
}

APInt APInt::sshl_ov(const APInt &ShAmt, bool &Overflow) const {
  Overflow = ShAmt.uge(getBitWidth());
  if (Overflow)
    return APInt(BitWidth, 0);

  if (isNonNegative()) // Don't allow sign change.
    Overflow = ShAmt.uge(countLeadingZeros());
  else
    Overflow = ShAmt.uge(countLeadingOnes());

  return *this << ShAmt;
}

APInt APInt::ushl_ov(const APInt &ShAmt, bool &Overflow) const {
  Overflow = ShAmt.uge(getBitWidth());
  if (Overflow)
    return APInt(BitWidth, 0);

  Overflow = ShAmt.ugt(countLeadingZeros());

  return *this << ShAmt;
}




void APInt::fromString(unsigned numbits, StringRef str, uint8_t radix) {
  // Check our assumptions here
  assert(!str.empty() && "Invalid string length");
  assert((radix == 10 || radix == 8 || radix == 16 || radix == 2 ||
          radix == 36) &&
         "Radix should be 2, 8, 10, 16, or 36!");

  StringRef::iterator p = str.begin();
  size_t slen = str.size();
  bool isNeg = *p == '-';
  if (*p == '-' || *p == '+') {
    p++;
    slen--;
    assert(slen && "String is only a sign, needs a value.");
  }
  assert((slen <= numbits || radix != 2) && "Insufficient bit width");
  assert(((slen-1)*3 <= numbits || radix != 8) && "Insufficient bit width");
  assert(((slen-1)*4 <= numbits || radix != 16) && "Insufficient bit width");
  assert((((slen-1)*64)/22 <= numbits || radix != 10) &&
         "Insufficient bit width");

  // Allocate memory
  if (!isSingleWord())
    pVal = getClearedMemory(getNumWords());

  // Figure out if we can shift instead of multiply
  unsigned shift = (radix == 16 ? 4 : radix == 8 ? 3 : radix == 2 ? 1 : 0);

  // Set up an APInt for the radix multiplier outside the loop so we don't
  // constantly construct/destruct it.
  APInt apradix(getBitWidth(), radix);

  // Enter digit traversal loop
  for (StringRef::iterator e = str.end(); p != e; ++p) {
    unsigned digit = getDigit(*p, radix);
    assert(digit < radix && "Invalid character in digit string");

    // Shift or multiply the value by the radix
    if (slen > 1) {
      if (shift)
        *this <<= shift;
      else
        *this *= apradix;
    }

    // Add in the digit we just interpreted
    *this += digit;
  }
  // If its negative, put it in two's complement form
  if (isNeg) {
    --(*this);
    this->flipAllBits();
  }
}

void APInt::toString(SmallVectorImpl<char> &Str, unsigned Radix,
                     bool Signed, bool formatAsCLiteral) const {
  assert((Radix == 10 || Radix == 8 || Radix == 16 || Radix == 2 ||
          Radix == 36) &&
         "Radix should be 2, 8, 10, 16, or 36!");

  const char *Prefix = "";
  if (formatAsCLiteral) {
    switch (Radix) {
      case 2:
        // Binary literals are a non-standard extension added in gcc 4.3:
        // http://gcc.gnu.org/onlinedocs/gcc-4.3.0/gcc/Binary-constants.html
        Prefix = "0b";
        break;
      case 8:
        Prefix = "0";
        break;
      case 10:
        break; // No prefix
      case 16:
        Prefix = "0x";
        break;
      default:
        llvm_unreachable("Invalid radix!");
    }
  }

  // First, check for a zero value and just short circuit the logic below.
  if (*this == 0) {
    while (*Prefix) {
      Str.push_back(*Prefix);
      ++Prefix;
    };
    Str.push_back('0');
    return;
  }

  static const char Digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  if (isSingleWord()) {
    char Buffer[65];
    char *BufPtr = Buffer+65;

    uint64_t N;
    if (!Signed) {
      N = getZExtValue();
    } else {
      int64_t I = getSExtValue();
      if (I >= 0) {
        N = I;
      } else {
        Str.push_back('-');
        N = -(uint64_t)I;
      }
    }

    while (*Prefix) {
      Str.push_back(*Prefix);
      ++Prefix;
    };

    while (N) {
      *--BufPtr = Digits[N % Radix];
      N /= Radix;
    }
    Str.append(BufPtr, Buffer+65);
    return;
  }

  APInt Tmp(*this);

  if (Signed && isNegative()) {
    // They want to print the signed version and it is a negative value
    // Flip the bits and add one to turn it into the equivalent positive
    // value and put a '-' in the result.
    Tmp.flipAllBits();
    ++Tmp;
    Str.push_back('-');
  }

  while (*Prefix) {
    Str.push_back(*Prefix);
    ++Prefix;
  };

  // We insert the digits backward, then reverse them to get the right order.
  unsigned StartDig = Str.size();

  // For the 2, 8 and 16 bit cases, we can just shift instead of divide
  // because the number of bits per digit (1, 3 and 4 respectively) divides
  // equally.  We just shift until the value is zero.
  if (Radix == 2 || Radix == 8 || Radix == 16) {
    // Just shift tmp right for each digit width until it becomes zero
    unsigned ShiftAmt = (Radix == 16 ? 4 : (Radix == 8 ? 3 : 1));
    unsigned MaskAmt = Radix - 1;

    while (Tmp != 0) {
      unsigned Digit = unsigned(Tmp.getRawData()[0]) & MaskAmt;
      Str.push_back(Digits[Digit]);
      Tmp.lshrInPlace(ShiftAmt);
    }
  } else {
    APInt divisor(Radix == 10? 4 : 8, Radix);
    while (Tmp != 0) {
      APInt APdigit(1, 0);
      APInt tmp2(Tmp.getBitWidth(), 0);
      divide(Tmp, Tmp.getNumWords(), divisor, divisor.getNumWords(), &tmp2,
             &APdigit);
      unsigned Digit = (unsigned)APdigit.getZExtValue();
      assert(Digit < Radix && "divide failed");
      Str.push_back(Digits[Digit]);
      Tmp = tmp2;
    }
  }

  // Reverse the digits before returning.
  std::reverse(Str.begin()+StartDig, Str.end());
}

/// Returns the APInt as a std::string. Note that this is an inefficient method.
/// It is better to pass in a SmallVector/SmallString to the methods above.
std::string APInt::toString(unsigned Radix = 10, bool Signed = true) const {
  SmallString<40> S;
  toString(S, Radix, Signed, /* formatAsCLiteral = */false);
  return S.str();
}

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
LLVM_DUMP_METHOD void APInt::dump() const {
  SmallString<40> S, U;
  this->toStringUnsigned(U);
  this->toStringSigned(S);
  dbgs() << "APInt(" << BitWidth << "b, "
         << U << "u " << S << "s)\n";
}
#endif

void APInt::print(raw_ostream &OS, bool isSigned) const {
  SmallString<40> S;
  this->toString(S, 10, isSigned, /* formatAsCLiteral = */false);
  OS << S;
}

// This implements a variety of operations on a representation of
// arbitrary precision, two's-complement, bignum integer values.

// Assumed by lowHalf, highHalf, partMSB and partLSB.  A fairly safe
// and unrestricting assumption.
static_assert(APInt::APINT_BITS_PER_WORD % 2 == 0,
              "Part width must be divisible by 2!");

/* Some handy functions local to this file.  */

/* Returns the integer part with the least significant BITS set.
   BITS cannot be zero.  */
static inline APInt::WordType lowBitMask(unsigned bits) {
  assert(bits != 0 && bits <= APInt::APINT_BITS_PER_WORD);

  return ~(APInt::WordType) 0 >> (APInt::APINT_BITS_PER_WORD - bits);
}

/* Returns the value of the lower half of PART.  */
static inline APInt::WordType lowHalf(APInt::WordType part) {
  return part & lowBitMask(APInt::APINT_BITS_PER_WORD / 2);
}

/* Returns the value of the upper half of PART.  */
static inline APInt::WordType highHalf(APInt::WordType part) {
  return part >> (APInt::APINT_BITS_PER_WORD / 2);
}

/* Returns the bit number of the most significant set bit of a part.
   If the input number has no bits set -1U is returned.  */
static unsigned partMSB(APInt::WordType value) {
  return findLastSet(value, ZB_Max);
}

/* Returns the bit number of the least significant set bit of a
   part.  If the input number has no bits set -1U is returned.  */
static unsigned partLSB(APInt::WordType value) {
  return findFirstSet(value, ZB_Max);
}

/* Sets the least significant part of a bignum to the input value, and
   zeroes out higher parts.  */
void APInt::tcSet(WordType *dst, WordType part, unsigned parts) {
  assert(parts > 0);

  dst[0] = part;
  for (unsigned i = 1; i < parts; i++)
    dst[i] = 0;
}

/* Assign one bignum to another.  */
void APInt::tcAssign(WordType *dst, const WordType *src, unsigned parts) {
  for (unsigned i = 0; i < parts; i++)
    dst[i] = src[i];
}

/* Returns true if a bignum is zero, false otherwise.  */
bool APInt::tcIsZero(const WordType *src, unsigned parts) {
  for (unsigned i = 0; i < parts; i++)
    if (src[i])
      return false;

  return true;
}

/* Extract the given bit of a bignum; returns 0 or 1.  */
int APInt::tcExtractBit(const WordType *parts, unsigned bit) {
  return (parts[whichWord(bit)] & maskBit(bit)) != 0;
}

/* Set the given bit of a bignum. */
void APInt::tcSetBit(WordType *parts, unsigned bit) {
  parts[whichWord(bit)] |= maskBit(bit);
}

/* Clears the given bit of a bignum. */
void APInt::tcClearBit(WordType *parts, unsigned bit) {
  parts[whichWord(bit)] &= ~maskBit(bit);
}

/* Returns the bit number of the least significant set bit of a
   number.  If the input number has no bits set -1U is returned.  */
unsigned APInt::tcLSB(const WordType *parts, unsigned n) {
  for (unsigned i = 0; i < n; i++) {
    if (parts[i] != 0) {
      unsigned lsb = partLSB(parts[i]);

      return lsb + i * APINT_BITS_PER_WORD;
    }
  }

  return -1U;
}

/* Returns the bit number of the most significant set bit of a number.
   If the input number has no bits set -1U is returned.  */
unsigned APInt::tcMSB(const WordType *parts, unsigned n) {
  do {
    --n;

    if (parts[n] != 0) {
      unsigned msb = partMSB(parts[n]);

      return msb + n * APINT_BITS_PER_WORD;
    }
  } while (n);

  return -1U;
}

/* Copy the bit vector of width srcBITS from SRC, starting at bit
   srcLSB, to DST, of dstCOUNT parts, such that the bit srcLSB becomes
   the least significant bit of DST.  All high bits above srcBITS in
   DST are zero-filled.  */
void
APInt::tcExtract(WordType *dst, unsigned dstCount, const WordType *src,
                 unsigned srcBits, unsigned srcLSB) {
  unsigned dstParts = (srcBits + APINT_BITS_PER_WORD - 1) / APINT_BITS_PER_WORD;
  assert(dstParts <= dstCount);

  unsigned firstSrcPart = srcLSB / APINT_BITS_PER_WORD;
  tcAssign (dst, src + firstSrcPart, dstParts);

  unsigned shift = srcLSB % APINT_BITS_PER_WORD;
  tcShiftRight (dst, dstParts, shift);

  /* We now have (dstParts * APINT_BITS_PER_WORD - shift) bits from SRC
     in DST.  If this is less that srcBits, append the rest, else
     clear the high bits.  */
  unsigned n = dstParts * APINT_BITS_PER_WORD - shift;
  if (n < srcBits) {
    WordType mask = lowBitMask (srcBits - n);
    dst[dstParts - 1] |= ((src[firstSrcPart + dstParts] & mask)
                          << n % APINT_BITS_PER_WORD);
  } else if (n > srcBits) {
    if (srcBits % APINT_BITS_PER_WORD)
      dst[dstParts - 1] &= lowBitMask (srcBits % APINT_BITS_PER_WORD);
  }

  /* Clear high parts.  */
  while (dstParts < dstCount)
    dst[dstParts++] = 0;
}

/* DST += RHS + C where C is zero or one.  Returns the carry flag.  */
APInt::WordType APInt::tcAdd(WordType *dst, const WordType *rhs,
                             WordType c, unsigned parts) {
  assert(c <= 1);

  for (unsigned i = 0; i < parts; i++) {
    WordType l = dst[i];
    if (c) {
      dst[i] += rhs[i] + 1;
      c = (dst[i] <= l);
    } else {
      dst[i] += rhs[i];
      c = (dst[i] < l);
    }
  }

  return c;
}

/// This function adds a single "word" integer, src, to the multiple
/// "word" integer array, dst[]. dst[] is modified to reflect the addition and
/// 1 is returned if there is a carry out, otherwise 0 is returned.
/// @returns the carry of the addition.
APInt::WordType APInt::tcAddPart(WordType *dst, WordType src,
                                 unsigned parts) {
  for (unsigned i = 0; i < parts; ++i) {
    dst[i] += src;
    if (dst[i] >= src)
      return 0; // No need to carry so exit early.
    src = 1; // Carry one to next digit.
  }

  return 1;
}

/* DST -= RHS + C where C is zero or one.  Returns the carry flag.  */
APInt::WordType APInt::tcSubtract(WordType *dst, const WordType *rhs,
                                  WordType c, unsigned parts) {
  assert(c <= 1);

  for (unsigned i = 0; i < parts; i++) {
    WordType l = dst[i];
    if (c) {
      dst[i] -= rhs[i] + 1;
      c = (dst[i] >= l);
    } else {
      dst[i] -= rhs[i];
      c = (dst[i] > l);
    }
  }

  return c;
}

/// This function subtracts a single "word" (64-bit word), src, from
/// the multi-word integer array, dst[], propagating the borrowed 1 value until
/// no further borrowing is needed or it runs out of "words" in dst.  The result
/// is 1 if "borrowing" exhausted the digits in dst, or 0 if dst was not
/// exhausted. In other words, if src > dst then this function returns 1,
/// otherwise 0.
/// @returns the borrow out of the subtraction
APInt::WordType APInt::tcSubtractPart(WordType *dst, WordType src,
                                      unsigned parts) {
  for (unsigned i = 0; i < parts; ++i) {
    WordType Dst = dst[i];
    dst[i] -= src;
    if (src <= Dst)
      return 0; // No need to borrow so exit early.
    src = 1; // We have to "borrow 1" from next "word"
  }

  return 1;
}

/* Negate a bignum in-place.  */
void APInt::tcNegate(WordType *dst, unsigned parts) {
  tcComplement(dst, parts);
  tcIncrement(dst, parts);
}

/*  DST += SRC * MULTIPLIER + CARRY   if add is true
    DST  = SRC * MULTIPLIER + CARRY   if add is false

    Requires 0 <= DSTPARTS <= SRCPARTS + 1.  If DST overlaps SRC
    they must start at the same point, i.e. DST == SRC.

    If DSTPARTS == SRCPARTS + 1 no overflow occurs and zero is
    returned.  Otherwise DST is filled with the least significant
    DSTPARTS parts of the result, and if all of the omitted higher
    parts were zero return zero, otherwise overflow occurred and
    return one.  */
int APInt::tcMultiplyPart(WordType *dst, const WordType *src,
                          WordType multiplier, WordType carry,
                          unsigned srcParts, unsigned dstParts,
                          bool add) {
  /* Otherwise our writes of DST kill our later reads of SRC.  */
  assert(dst <= src || dst >= src + srcParts);
  assert(dstParts <= srcParts + 1);

  /* N loops; minimum of dstParts and srcParts.  */
  unsigned n = dstParts < srcParts ? dstParts: srcParts;

  unsigned i;
  for (i = 0; i < n; i++) {
    WordType low, mid, high, srcPart;

      /* [ LOW, HIGH ] = MULTIPLIER * SRC[i] + DST[i] + CARRY.

         This cannot overflow, because

         (n - 1) * (n - 1) + 2 (n - 1) = (n - 1) * (n + 1)

         which is less than n^2.  */

    srcPart = src[i];

    if (multiplier == 0 || srcPart == 0) {
      low = carry;
      high = 0;
    } else {
      low = lowHalf(srcPart) * lowHalf(multiplier);
      high = highHalf(srcPart) * highHalf(multiplier);

      mid = lowHalf(srcPart) * highHalf(multiplier);
      high += highHalf(mid);
      mid <<= APINT_BITS_PER_WORD / 2;
      if (low + mid < low)
        high++;
      low += mid;

      mid = highHalf(srcPart) * lowHalf(multiplier);
      high += highHalf(mid);
      mid <<= APINT_BITS_PER_WORD / 2;
      if (low + mid < low)
        high++;
      low += mid;

      /* Now add carry.  */
      if (low + carry < low)
        high++;
      low += carry;
    }

    if (add) {
      /* And now DST[i], and store the new low part there.  */
      if (low + dst[i] < low)
        high++;
      dst[i] += low;
    } else
      dst[i] = low;

    carry = high;
  }

  if (i < dstParts) {
    /* Full multiplication, there is no overflow.  */
    assert(i + 1 == dstParts);
    dst[i] = carry;
    return 0;
  } else {
    /* We overflowed if there is carry.  */
    if (carry)
      return 1;

    /* We would overflow if any significant unwritten parts would be
       non-zero.  This is true if any remaining src parts are non-zero
       and the multiplier is non-zero.  */
    if (multiplier)
      for (; i < srcParts; i++)
        if (src[i])
          return 1;

    /* We fitted in the narrow destination.  */
    return 0;
  }
}

/* DST = LHS * RHS, where DST has the same width as the operands and
   is filled with the least significant parts of the result.  Returns
   one if overflow occurred, otherwise zero.  DST must be disjoint
   from both operands.  */
int APInt::tcMultiply(WordType *dst, const WordType *lhs,
                      const WordType *rhs, unsigned parts) {
  assert(dst != lhs && dst != rhs);

  int overflow = 0;
  tcSet(dst, 0, parts);

  for (unsigned i = 0; i < parts; i++)
    overflow |= tcMultiplyPart(&dst[i], lhs, rhs[i], 0, parts,
                               parts - i, true);

  return overflow;
}

/* DST = LHS * RHS, where DST has width the sum of the widths of the
   operands.  No overflow occurs.  DST must be disjoint from both
   operands.  Returns the number of parts required to hold the
   result.  */
unsigned APInt::tcFullMultiply(WordType *dst, const WordType *lhs,
                               const WordType *rhs, unsigned lhsParts,
                               unsigned rhsParts) {
  /* Put the narrower number on the LHS for less loops below.  */
  if (lhsParts > rhsParts) {
    return tcFullMultiply (dst, rhs, lhs, rhsParts, lhsParts);
  } else {
    assert(dst != lhs && dst != rhs);

    tcSet(dst, 0, rhsParts);

    for (unsigned i = 0; i < lhsParts; i++)
      tcMultiplyPart(&dst[i], rhs, lhs[i], 0, rhsParts, rhsParts + 1, true);

    unsigned n = lhsParts + rhsParts;

    return n - (dst[n - 1] == 0);
  }
}

/* If RHS is zero LHS and REMAINDER are left unchanged, return one.
   Otherwise set LHS to LHS / RHS with the fractional part discarded,
   set REMAINDER to the remainder, return zero.  i.e.

   OLD_LHS = RHS * LHS + REMAINDER

   SCRATCH is a bignum of the same size as the operands and result for
   use by the routine; its contents need not be initialized and are
   destroyed.  LHS, REMAINDER and SCRATCH must be distinct.
*/
int APInt::tcDivide(WordType *lhs, const WordType *rhs,
                    WordType *remainder, WordType *srhs,
                    unsigned parts) {
  assert(lhs != remainder && lhs != srhs && remainder != srhs);

  unsigned shiftCount = tcMSB(rhs, parts) + 1;
  if (shiftCount == 0)
    return true;

  shiftCount = parts * APINT_BITS_PER_WORD - shiftCount;
  unsigned n = shiftCount / APINT_BITS_PER_WORD;
  WordType mask = (WordType) 1 << (shiftCount % APINT_BITS_PER_WORD);

  tcAssign(srhs, rhs, parts);
  tcShiftLeft(srhs, parts, shiftCount);
  tcAssign(remainder, lhs, parts);
  tcSet(lhs, 0, parts);

  /* Loop, subtracting SRHS if REMAINDER is greater and adding that to
     the total.  */
  for (;;) {
      int compare;

      compare = tcCompare(remainder, srhs, parts);
      if (compare >= 0) {
        tcSubtract(remainder, srhs, 0, parts);
        lhs[n] |= mask;
      }

      if (shiftCount == 0)
        break;
      shiftCount--;
      tcShiftRight(srhs, parts, 1);
      if ((mask >>= 1) == 0) {
        mask = (WordType) 1 << (APINT_BITS_PER_WORD - 1);
        n--;
      }
  }

  return false;
}

/// Shift a bignum left Cound bits in-place. Shifted in bits are zero. There are
/// no restrictions on Count.
void APInt::tcShiftLeft(WordType *Dst, unsigned Words, unsigned Count) {
  // Don't bother performing a no-op shift.
  if (!Count)
    return;

  /* WordShift is the inter-part shift; BitShift is is intra-part shift.  */
  unsigned WordShift = std::min(Count / APINT_BITS_PER_WORD, Words);
  unsigned BitShift = Count % APINT_BITS_PER_WORD;

  // Fastpath for moving by whole words.
  if (BitShift == 0) {
    std::memmove(Dst + WordShift, Dst, (Words - WordShift) * APINT_WORD_SIZE);
  } else {
    while (Words-- > WordShift) {
      Dst[Words] = Dst[Words - WordShift] << BitShift;
      if (Words > WordShift)
        Dst[Words] |=
          Dst[Words - WordShift - 1] >> (APINT_BITS_PER_WORD - BitShift);
    }
  }

  // Fill in the remainder with 0s.
  std::memset(Dst, 0, WordShift * APINT_WORD_SIZE);
}

/// Shift a bignum right Count bits in-place. Shifted in bits are zero. There
/// are no restrictions on Count.
void APInt::tcShiftRight(WordType *Dst, unsigned Words, unsigned Count) {
  // Don't bother performing a no-op shift.
  if (!Count)
    return;

  // WordShift is the inter-part shift; BitShift is is intra-part shift.
  unsigned WordShift = std::min(Count / APINT_BITS_PER_WORD, Words);
  unsigned BitShift = Count % APINT_BITS_PER_WORD;

  unsigned WordsToMove = Words - WordShift;
  // Fastpath for moving by whole words.
  if (BitShift == 0) {
    std::memmove(Dst, Dst + WordShift, WordsToMove * APINT_WORD_SIZE);
  } else {
    for (unsigned i = 0; i != WordsToMove; ++i) {
      Dst[i] = Dst[i + WordShift] >> BitShift;
      if (i + 1 != WordsToMove)
        Dst[i] |= Dst[i + WordShift + 1] << (APINT_BITS_PER_WORD - BitShift);
    }
  }

  // Fill in the remainder with 0s.
  std::memset(Dst + WordsToMove, 0, WordShift * APINT_WORD_SIZE);
}

/* Bitwise and of two bignums.  */
void APInt::tcAnd(WordType *dst, const WordType *rhs, unsigned parts) {
  for (unsigned i = 0; i < parts; i++)
    dst[i] &= rhs[i];
}

/* Bitwise inclusive or of two bignums.  */
void APInt::tcOr(WordType *dst, const WordType *rhs, unsigned parts) {
  for (unsigned i = 0; i < parts; i++)
    dst[i] |= rhs[i];
}

/* Bitwise exclusive or of two bignums.  */
void APInt::tcXor(WordType *dst, const WordType *rhs, unsigned parts) {
  for (unsigned i = 0; i < parts; i++)
    dst[i] ^= rhs[i];
}

/* Complement a bignum in-place.  */
void APInt::tcComplement(WordType *dst, unsigned parts) {
  for (unsigned i = 0; i < parts; i++)
    dst[i] = ~dst[i];
}

/* Comparison (unsigned) of two bignums.  */
int APInt::tcCompare(const WordType *lhs, const WordType *rhs,
                     unsigned parts) {
  while (parts) {
    parts--;
    if (lhs[parts] == rhs[parts])
      continue;

    return (lhs[parts] > rhs[parts]) ? 1 : -1;
  }

  return 0;
}

/* Set the least significant BITS bits of a bignum, clear the
   rest.  */
void APInt::tcSetLeastSignificantBits(WordType *dst, unsigned parts,
                                      unsigned bits) {
  unsigned i = 0;
  while (bits > APINT_BITS_PER_WORD) {
    dst[i++] = ~(WordType) 0;
    bits -= APINT_BITS_PER_WORD;
  }

  if (bits)
    dst[i++] = ~(WordType) 0 >> (APINT_BITS_PER_WORD - bits);

  while (i < parts)
    dst[i++] = 0;
}
