/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2014 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * RELIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RELIC. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @defgroup ed Edwards curves over prime fields
 */

/**
 * @file
 *
 * Interface of the module for arithmetic on elliptic twisted Edwards curves.
 *
 * @version $Id$
 * @ingroup ed
 */

 #ifndef RELIC_ED_H
 #define RELIC_ED_H

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

/**
 * Prime elliptic twisted Edwards curve identifiers.
 */
enum {
  /** ED25519 prime curve. */
  CURVE_ED25519 = 1
};

/*============================================================================*/
/* Type definitions                                                           */
/*============================================================================*/

/**
 * Represents an elliptic curve point over a prime field.
 */
typedef struct {
#if ALLOC == STATIC
  /** The first coordinate. */
  fp_t x;
  /** The second coordinate. */
  fp_t y;
  /** The third coordinate (projective representation). */
  fp_t z;
#elif ALLOC == DYNAMIC || ALLOC == STACK || ALLOC == AUTO
  /** The first coordinate. */
  fp_st x;
  /** The second coordinate. */
  fp_st y;
  /** The third coordinate (projective representation). */
  fp_st z;
#endif
  /** Flag to indicate that this point is normalized. */
  int norm;
} ed_st;

/**
 * Pointer to an elliptic curve point.
 */
#if ALLOC == AUTO
typedef ed_st ed_t[1];
#else
typedef ed_st *ed_t;
#endif

/*============================================================================*/
/* Precomputaion table                                                        */
/*============================================================================*/
/**
 * Size of a precomputation table using the binary method.
 */
#define ED_TABLE_BASIC		(FP_BITS + 1)

/**
 * Size of a precomputation table using Yao's windowing method.
 */
#define ED_TABLE_YAOWI      (FP_BITS / ED_DEPTH + 1)

/**
 * Size of a precomputation table using the NAF windowing method.
 */
#define ED_TABLE_NAFWI      (FP_BITS / ED_DEPTH + 1)

/**
 * Size of a precomputation table using the single-table comb method.
 */
#define ED_TABLE_COMBS      (1 << ED_DEPTH)

/**
 * Size of a precomputation table using the double-table comb method.
 */
#define ED_TABLE_COMBD		(1 << (ED_DEPTH + 1))

/**
 * Size of a precomputation table using the w-(T)NAF method.
 */
#define ED_TABLE_LWNAF		(1 << (ED_DEPTH - 2))

/**
 * Size of a precomputation table using the chosen algorithm.
 */
#if ED_FIX == BASIC
#define ED_TABLE			ED_TABLE_BASIC
#elif ED_FIX == YAOWI
#define ED_TABLE			ED_TABLE_YAOWI
#elif ED_FIX == NAFWI
#define ED_TABLE			ED_TABLE_NAFWI
#elif ED_FIX == COMBS
#define ED_TABLE			ED_TABLE_COMBS
#elif ED_FIX == COMBD
#define ED_TABLE			ED_TABLE_COMBD
#elif ED_FIX == LWNAF
#define ED_TABLE			ED_TABLE_LWNAF
#endif

/**
 * Maximum size of a precomputation table.
 */
#ifdef STRIP
#define ED_TABLE_MAX ED_TABLE
#else
#define ED_TABLE_MAX MAX(ED_TABLE_BASIC, ED_TABLE_COMBD)
#endif

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

/**
 * Initializes a point on a prime elliptic twisted Edwards curve with a null value.
 *
 * @param[out] A      - the point to initialize.
 */
#if ALLOC == AUTO
#define ed_null(A)        /* empty */
#else
#define ed_null(A)    A = NULL;
#endif

/**
 * Calls a function to allocate a point on a prime elliptic twisted Edwards curve.
 *
 * @param[out] A      - the new point.
 * @throw ERR_NO_MEMORY   - if there is no available memory.
 */
#if ALLOC == DYNAMIC
#define ed_new(A)                             \
  A = (ed_t)calloc(1, sizeof(ed_st));                   \
  if (A == NULL) {                            \
    THROW(ERR_NO_MEMORY);                       \
  }                                   \

#elif ALLOC == STATIC
#define ed_new(A)                             \
  A = (ed_t)alloca(sizeof(ed_st));                    \
  if (A == NULL) {                            \
    THROW(ERR_NO_MEMORY);                       \
  }                                   \
  fp_null((A)->x);                            \
  fp_null((A)->y);                            \
  fp_null((A)->z);                            \
  fp_new((A)->x);                             \
  fp_new((A)->y);                             \
  fp_new((A)->z);                             \

#elif ALLOC == AUTO
#define ed_new(A)       /* empty */

#elif ALLOC == STACK
#define ed_new(A)                             \
  A = (ed_t)alloca(sizeof(ed_st));                    \

#endif

/**
 * Calls a function to clean and free a point on a prime elliptic twisted Edwards curve.
 *
 * @param[out] A      - the point to free.
 */
#if ALLOC == DYNAMIC
#define ed_free(A)                              \
  if (A != NULL) {                            \
    free(A);                              \
    A = NULL;                             \
  }

#elif ALLOC == STATIC
#define ed_free(A)                              \
  if (A != NULL) {                            \
    fp_free((A)->x);                          \
    fp_free((A)->y);                          \
    fp_free((A)->z);                          \
    A = NULL;                             \
  }                                   \

#elif ALLOC == AUTO
#define ed_free(A)        /* empty */

#elif ALLOC == STACK
#define ed_free(A)                              \
  A = NULL;                               \

#endif


/**
 * Configures a prime elliptic twisted Edwards curve by its parameter identifier.
 *
 * @param				- the parameter identifier.
 */
void ed_param_set(int param);

/**
 * Configures some set of curve parameters for the current security level.
 */
int ed_param_set_any(void);

/**
 * Returns the parameter identifier of the currently configured prime elliptic
 * curve.
 *
 * @return the parameter identifier.
 */
int ed_param_get(void);

/**
 * Returns the order of the group of points in the prime elliptic twisted Edwards curve.
 *
 * @param[out] r      - the returned order.
 */
void ed_curve_get_ord(bn_t r);

/**
 * Returns the generator of the group of points in the prime elliptic twisted Edwards curve.
 *
 * @param[out] g      - the returned generator.
 */
void ed_curve_get_gen(ed_t g);

/**
 * Returns the cofactor of the prime elliptic twisted Edwards curve.
 *
 * @param[out] n      - the returned cofactor.
 */
void ed_curve_get_cof(bn_t h);

/**
 * Prints the current configured prime elliptic twisted Edwards curve.
 */
void ed_param_print(void);

/**
 * Returns the current security level.
 */
int ed_param_level(void);

/**
 * Recovers the x coordinate of and Edwards curve point given y coordinate and d.
 */
void ed_recover_x(fp_t x, const fp_t y, const fp_t d);

/**
 * Assigns a random value to a prime elliptic twisted Edwards curve point.
 *
 * @param[out] p      - the prime elliptic twisted Edwards curve point to assign.
 */
void ed_rand(ed_t p);

/**
 * Copies the second argument to the first argument.
 *
 * @param[out] q      - the result.
 * @param[in] p       - the prime elliptic twisted Edwards curve point to copy.
 */
void ed_copy(ed_t r, const ed_t p);

/**
 * Compares two prime elliptic twisted Edwards curve points.
 *
 * @param[in] p       - the first prime elliptic twisted Edwards curve point.
 * @param[in] q       - the second prime elliptic twisted Edwards curve point.
 * @return CMP_EQ if p == q and CMP_NE if p != q.
 */
int ed_cmp(const ed_t p, const ed_t q);

/**
 * Assigns a prime elliptic twisted Edwards curve point to a point at the infinity.
 *
 * @param[out] p      - the point to assign.
 */
void ed_set_infty(ed_t p);

/**
 * Tests if a point on a prime elliptic twisted Edwards curve is at the infinity.
 *
 * @param[in] p       - the point to test.
 * @return 1 if the point is at infinity, 0 otherise.
 */
int ed_is_infty(const ed_t p);

/**
 * Negates a prime elliptic twisted Edwards curve point represented by projective coordinates.
 *
 * @param[out] r      - the result.
 * @param[in] p       - the point to negate.
 */
void ed_neg(ed_t r, const ed_t p);

/**
 * Adds two prime elliptic twisted Edwards curve points represented in projective coordinates.
 *
 * @param[out] r      - the result.
 * @param[in] p       - the first point to add.
 * @param[in] q       - the second point to add.
 */
void ed_add(ed_t r, const ed_t p, const ed_t q);

/**
 * Subtracts a prime elliptic twisted Edwards curve point from another, that is, compute
 * R = P - Q.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the first point.
 * @param[in] Q				- the second point.
 */
void ed_sub(ed_t r, const ed_t p, const ed_t q);

/**
 * Doubles a prime elliptic twisted Edwards curve point represented in projective coordinates.
 *
 * @param[out] r      - the result.
 * @param[in] p       - the point to double.
 */
void ed_dbl(ed_t r, const ed_t p);

/**
 * Converts a point to affine coordinates.
 *
 * @param[out] r      - the result.
 * @param[in] p       - the point to convert.
 */
void ed_norm(ed_t r, const ed_t p);

/**
 * Maps a byte array to a point in a prime elliptic twisted Edwards curve.
 *
 * @param[out] p			- the result.
 * @param[in] msg			- the byte array to map.
 * @param[in] len			- the array length in bytes.
 */
void ed_map(ed_t p, const uint8_t *msg, int len);

/**
 * Multiplies a prime elliptic curve point by an integer. Computes R = kP.
 *
 * @param[out] R			- the result.
 * @param[in] P				- the point to multiply.
 * @param[in] K				- the integer.
 */
#if ED_MUL == BASIC
#define ed_mul(R, P, K)		ed_mul_basic(R, P, K)
#elif ED_MUL == SLIDE
#define ed_mul(R, P, K)		ed_mul_slide(R, P, K)
#elif ED_MUL == MONTY
#define ed_mul(R, P, K)		ed_mul_monty(R, P, K)
#elif ED_MUL == LWNAF
#define ed_mul(R, P, K)		ed_mul_lwnaf(R, P, K)
#endif

/**
 * Builds a precomputation table for multiplying a fixed prime elliptic point.
 *
 * @param[out] T			- the precomputation table.
 * @param[in] P				- the point to multiply.
 */
#if ED_FIX == BASIC
#define ed_mul_pre(T, P)		ed_mul_pre_basic(T, P)
#elif ED_FIX == YAOWI
#define ed_mul_pre(T, P)		ed_mul_pre_yaowi(T, P)
#elif ED_FIX == NAFWI
#define ed_mul_pre(T, P)		ed_mul_pre_nafwi(T, P)
#elif ED_FIX == COMBS
#define ed_mul_pre(T, P)		ed_mul_pre_combs(T, P)
#elif ED_FIX == COMBD
#define ed_mul_pre(T, P)		ed_mul_pre_combd(T, P)
#elif ED_FIX == LWNAF
#define ed_mul_pre(T, P)		ed_mul_pre_lwnaf(T, P)
#endif

/**
 * Multiplies a fixed prime elliptic point using a precomputation table.
 * Computes R = kP.
 *
 * @param[out] R			- the result.
 * @param[in] T				- the precomputation table.
 * @param[in] K				- the integer.
 */
#if ED_FIX == BASIC
#define ed_mul_fix(R, T, K)		ed_mul_fix_basic(R, T, K)
#elif ED_FIX == YAOWI
#define ed_mul_fix(R, T, K)		ed_mul_fix_yaowi(R, T, K)
#elif ED_FIX == NAFWI
#define ed_mul_fix(R, T, K)		ed_mul_fix_nafwi(R, T, K)
#elif ED_FIX == COMBS
#define ed_mul_fix(R, T, K)		ed_mul_fix_combs(R, T, K)
#elif ED_FIX == COMBD
#define ed_mul_fix(R, T, K)		ed_mul_fix_combd(R, T, K)
#elif ED_FIX == LWNAF
#define ed_mul_fix(R, T, K)		ed_mul_fix_lwnaf(R, T, K)
#endif

/**
 * Multiplies the generator of a prime elliptic twisted Edwards curve by an integer.
 *
 * @param[out] r      - the result.
 * @param[in] k       - the integer.
 */
void ed_mul_gen(ed_t r, const bn_t k);

/**
 * Multiplies a prime elliptic twisted Edwards curve point by a small integer.
 *
 * @param[out] r      - the result.
 * @param[in] p       - the point to multiply.
 * @param[in] k       - the integer.
 */
void ed_mul_dig(ed_t r, const ed_t p, dig_t k);

/**
 * Multiplies and adds two prime elliptic twisted Edwards curve points simultaneously. Computes
 * R = kP + mQ.
 *
 * @param[out] R      - the result.
 * @param[in] P       - the first point to multiply.
 * @param[in] K       - the first integer.
 * @param[in] Q       - the second point to multiply.
 * @param[in] M       - the second integer,
 */
void ed_mul_sim(ed_t r, const ed_t p, const bn_t k, const ed_t q,
    const bn_t m);

/**
 * Multiplies and adds the generator and a prime elliptic twisted Edwards curve point
 * simultaneously. Computes R = kG + mQ.
 *
 * @param[out] r      - the result.
 * @param[in] k       - the first integer.
 * @param[in] q       - the second point to multiply.
 * @param[in] m       - the second integer.
 */
void ed_mul_sim_gen(ed_t r, const bn_t k, const ed_t q, const bn_t m);

/**
 * Prints a prime elliptic twisted Edwards curve point.
 *
 * @param[in] p       - the prime elliptic curve point to print.
 */
void ed_print(const ed_t p);

/**
 * Tests if a point is in the curve.
 *
 * @param[in] p       - the point to test.
 */
int ed_is_valid(const ed_t p);

/**
 * Returns the number of bytes necessary to store a prime elliptic twisted Edwards curve point
 * with optional point compression.
 *
 * @param[in] a       - the prime field element.
 * @param[in] pack      - the flag to indicate compression.
 * @return the number of bytes.
 */
int ed_size_bin(const ed_t a, int pack);

/**
 * Reads a prime elliptic twisted Edwards curve point from a byte vector in big-endian format.
 *
 * @param[out] a      - the result.
 * @param[in] bin     - the byte vector.
 * @param[in] len     - the buffer capacity.
 * @throw ERR_NO_VALID    - if the encoded point is invalid.
 * @throw ERR_NO_BUFFER   - if the buffer capacity is invalid.
 */
void ed_read_bin(ed_t a, const uint8_t *bin, int len);

/**
 * Writes a prime elliptic twisted Edwards curve point to a byte vector in big-endian format
 * with optional point compression.
 *
 * @param[out] bin      - the byte vector.
 * @param[in] len     - the buffer capacity.
 * @param[in] a       - the prime elliptic curve point to write.
 * @param[in] pack      - the flag to indicate point compression.
 * @throw ERR_NO_BUFFER   - if the buffer capacity is invalid.
 */
void ed_write_bin(uint8_t *bin, int len, const ed_t a, int pack);

/**
 * Multiplies a prime elliptic point by an integer using the binary method.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ed_mul_basic(ed_t r, const ed_t p, const bn_t k);

/**
 * Multiplies a prime elliptic point by an integer using the sliding window
 * method.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ed_mul_slide(ed_t r, const ed_t p, const bn_t k);

/**
 * Multiplies a prime elliptic point by an integer using the constant-time
 * Montgomery laddering point multiplication method.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ed_mul_monty(ed_t r, const ed_t p, const bn_t k);

/**
 * Multiplies a prime elliptic point by an integer using the w-NAF method.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ed_mul_lwnaf(ed_t r, const ed_t p, const bn_t k);

/**
 * Multiplies a prime elliptic point by an integer using a regular method.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 * @param[in] k				- the integer.
 */
void ed_mul_lwreg(ed_t r, const ed_t p, const bn_t k);

#endif
