/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2019 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or modify it under the
 * terms of the version 2.1 (or later) of the GNU Lesser General Public License
 * as published by the Free Software Foundation; or version 2.0 of the Apache
 * License as published by the Apache Software Foundation. See the LICENSE files
 * for more details.
 *
 * RELIC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the LICENSE files for more details.
 *
 * You should have received a copy of the GNU Lesser General Public or the
 * Apache License along with RELIC. If not, see <https://www.gnu.org/licenses/>
 * or <https://www.apache.org/licenses/>.
 */

/**
 * @file
 *
 * Implementation of hashing to a prime elliptic curve.
 *
 * @ingroup ep
 */

#include "relic_core.h"
#include "relic_md.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Optimized Shallue–van de Woestijne encoding from Section 3 of
 * "Fast and simple constant-time hashing to the BLS12-381 elliptic curve".
 */
static void ep_sw_b12(ep_t p, const fp_t t, int u, int negate) {
	fp_t t0, t1, t2, t3;

	fp_null(t0);
	fp_null(t1);
	fp_null(t2);
	fp_null(t3);

	TRY {
		fp_new(t0);
		fp_new(t1);
		fp_new(t2);
		fp_new(t3);

		/* t0 = t^2. */
		fp_sqr(t0, t);
		/* Compute f(u) such that u^3 + b is a square. */
		fp_set_dig(p->x, -u);
		fp_neg(p->x, p->x);
		ep_rhs(t1, p);
		/* Compute t1 = (-f(u) + t^2), t2 = t1 * t^2 and invert if non-zero. */
		fp_add(t1, t1, t0);
		fp_mul(t2, t1, t0);
		if (!fp_is_zero(t2)) {
			/* Compute inverse of u^3 * t2 and fix later. */
			fp_mul(t2, t2, p->x);
			fp_mul(t2, t2, p->x);
			fp_mul(t2, t2, p->x);
			fp_inv(t2, t2);
		}
		/* Compute t0 = t^4 * u * sqrt(-3)/t2. */
		fp_sqr(t0, t0);
		fp_mul(t0, t0, t2);
		fp_mul(t0, t0, p->x);
		fp_mul(t0, t0, p->x);
		fp_mul(t0, t0, p->x);
		/* Compute constant u * sqrt(-3). */
		fp_copy(t3, core_get()->srm3);
		for (int i = 1; i < -u; i++) {
			fp_add(t3, t3, core_get()->srm3);
		}
		fp_mul(t0, t0, t3);
		/* Compute (u * sqrt(-3) + u)/2 - t0. */
		fp_add_dig(p->x, t3, -u);
		fp_hlv(p->y, p->x);
		fp_sub(p->x, p->y, t0);
		ep_rhs(p->y, p);
		if (!fp_srt(p->y, p->y)) {
			/* Now try t0 - (u * sqrt(-3) - u)/2. */
			fp_sub_dig(p->x, t3, -u);
			fp_hlv(p->y, p->x);
			fp_sub(p->x, t0, p->y);
			ep_rhs(p->y, p);
			if (!fp_srt(p->y, p->y)) {
				/* Finally, try (u - t1^2 / t2). */
				fp_sqr(p->x, t1);
				fp_mul(p->x, p->x, t1);
				fp_mul(p->x, p->x, t2);
				fp_sub_dig(p->x, p->x, -u);
				ep_rhs(p->y, p);
				fp_srt(p->y, p->y);
			}
		}
		if (negate) {
			fp_neg(p->y, p->y);
		}
		fp_set_dig(p->z, 1);
		p->norm = 1;
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp_free(t0);
		fp_free(t1);
		fp_free(t2);
		fp_free(t3);
	}
}

/**
 * Simplified SWU mapping from Section 4 of
 * "Fast and simple constant-time hashing to the BLS12-381 Elliptic Curve"
 */
static void ep_sswu_abNeq0(ep_t p, const fp_t t, int u, int negate) {
    fp_t t0, t1, t2, t3, t4;
    fp_null(t0);
    fp_null(t1);
    fp_null(t2);
    fp_null(t3);
    fp_null(t4);

    TRY {
        fp_new(t0);
        fp_new(t1);
        fp_new(t2);
        fp_new(t3);
        fp_new(t4);
        if (ep_curve_opt_a() == RLC_ZERO || ep_curve_opt_b() == RLC_ZERO) {
            THROW(ERR_NO_VALID);
        }

        /* compute u and -u as field elms */
        const int e0 = u < 0;
        u = e0 ? -u : u;
        fp_set_dig(t3, u);                      /* t3 = abs(u) */
        fp_neg(t4, t3);                         /* t4 = - abs(u) */
        dv_swap_cond(t3, t4, RLC_FP_DIGS, e0);  /* t3 = u, t4 = -u */

        /* start computing the map */
        fp_sqr(t0, t);
        fp_mul(t0, t0, t3);                     /* t0 = u * t^2 */
        fp_sqr(t1, t0);                         /* t1 = u^2 * t^4 */
        fp_add(t2, t1, t0);                     /* t2 = u^2 * t^4 + u * t^2 */

        /* handle the exceptional cases and simultaneously invert a */
        const int e1 = fp_is_zero(t2);
        dv_copy_cond(t2, t4, RLC_FP_DIGS, e1);  /* exceptional case: -u instead of u^2t^4 + ut^2 */
        fp_mul(t3, t2, ep_curve_get_a());       /* t3 = t2 * a */
        fp_inv(t4, t3);                         /* t4 is either -1/au or 1/a(u^2 * t^4 + u * t^2) */
        fp_mul(t3, t4, t2);                     /* t3 = 1/a */
        fp_mul(t2, t4, ep_curve_get_a());       /* t2 = -1/u or 1/(u^2 * t^4 + u*t^2) */
        fp_add_dig(t4, t2, 1);                  /* t4 = 1 + t2 */
        dv_copy_cond(t2, t4, RLC_FP_DIGS, e1 == 0); /* only add 1 if t2 != -1/u */

        /* compute -B / A */
        fp_neg(t3, t3);                     /* t3 = -1 / A */
        fp_mul(t3, t3, ep_curve_get_b());   /* t3 = -B / A */

        /* compute x1, g(x1) */
        fp_mul(p->x, t2, t3);       /* p->x = -B / A * (1 + 1 / (u^2 * t^4 + u * t^2)) */
        ep_rhs(p->y, p);            /* p->y = g(t2) */

        /* compute x2, g(x2) */
        fp_mul(t2, t0, p->x);       /* t2 = u * t^2 * x1 */
        fp_mul(t1, t0, t1);         /* t1 = u^3 * t^6 */
        fp_mul(t3, t1, p->y);       /* t5 = g(t2) = u^3 * t^6 * g(p->x) */

        /* XXX(rsw)
         * This should be done in constant time and without computing 2 sqrts.
         * Avoiding a second sqrt relies on knowing the 2-adicity of the modulus.
         */
        if (!fp_srt(p->y, p->y)) {
            /* try x2, g(x2) */
            fp_copy(p->x, t2);
            if (!fp_srt(p->y, t3)) {
                THROW(ERR_NO_VALID);
            }
        }
        if (negate) {
            fp_neg(p->y, p->y);
        }
        fp_set_dig(p->z, 1);
        p->norm = 1;
    }
    CATCH_ANY {
        THROW(ERR_CAUGHT);
    }
    FINALLY {
        fp_free(t0);
        fp_free(t1);
        fp_free(t2);
        fp_free(t3);
        fp_free(t4);
    }
}

/**
 * Based on the rust implementation of pairings, zkcrypto/pairing.
 * The algorithm is Shallue–van de Woestijne encoding from
 * Section 3 of "Indifferentiable Hashing to Barreto–Naehrig Curves"
 * from Fouque-Tibouchi: <https://www.di.ens.fr/~fouque/pub/latincrypt12.pdf>
 */
static void ep_sw_bn(ep_t p, const fp_t t, int u, int negate) {
	fp_t t0;
	fp_t t1;
	if (fp_is_zero(t)) {
		ep_set_infty(p);
		return;
	}

	fp_null(t0);
	fp_null(t1);

	TRY {
		fp_new(t0);
		fp_new(t1);

		/* w = t^2 + b + 1 */
		fp_sqr(t0, t);
		fp_add(t0, t0, ep_curve_get_b());
		fp_add_dig(t0, t0, 1);

		if (fp_is_zero(t0)) {
			ep_curve_get_gen(p);
			return;
		}

		/* (sqrt(-3) - u) / 2 */
		fp_copy(t1, core_get()->srm3);
		fp_sub_dig(t1, t1, -u);
		fp_hlv(t1, t1);

		fp_inv(t0, t0);
		fp_mul(t0, t0, core_get()->srm3);
		fp_mul(t0, t0, t);

		/* x1 = -wt + sqrt(-3) */
		fp_neg(p->x, t0);
		fp_mul(p->x, p->x, t);
		fp_add(p->x, p->x, t1);
		ep_rhs(p->y, p);
		if (!fp_srt(p->y, p->y)) {
			/* x2 = - x1 - u */
			fp_neg(p->x, p->x);
			fp_sub_dig(p->x, p->x, -u);
			ep_rhs(p->y, p);
			if (!fp_srt(p->y, p->y)) {
				/* x3 = (w^2 + u)/w^2 */
				fp_sqr(p->x, t0);
				fp_inv(p->x, p->x);
				fp_add_dig(p->x, p->x, -u);
				ep_rhs(p->y, p);
				fp_srt(p->y, p->y);
				p->norm = 0;
			}
		}

		if (negate) {
			fp_neg(p->y, p->y);
		}
		fp_set_dig(p->z, 1);
		p->norm = 1;
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp_free(t0);
		fp_free(t1);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ep_map(ep_t p, const uint8_t *msg, int len) {
	bn_t k, pm1o2;
	fp_t t;
	ep_t q;
	uint8_t digest[RLC_MD_LEN];
	int neg;

	bn_null(k);
	bn_null(pm1o2);
	fp_null(t);
	ep_null(q);

	TRY {
		bn_new(k);
		bn_new(pm1o2);
		fp_new(t);
		ep_new(q);

		pm1o2->sign = RLC_POS;
		pm1o2->used = RLC_FP_DIGS;
		dv_copy(pm1o2->dp, fp_prime_get(), RLC_FP_DIGS);
		bn_hlv(pm1o2, pm1o2);
		md_map(digest, msg, len);
		bn_read_bin(k, digest, RLC_MIN(RLC_FP_BYTES, RLC_MD_LEN));
		fp_prime_conv(t, k);
		fp_prime_back(k, t);
		neg = (bn_cmp(k, pm1o2) == RLC_LT ? 0 : 1);

		switch (ep_curve_is_pairf()) {
			case EP_BN:
				ep_sw_bn(p, t, -1, neg);
				md_map(digest, digest, RLC_MD_LEN);
				bn_read_bin(k, digest, RLC_MIN(RLC_FP_BYTES, RLC_MD_LEN));
				fp_prime_conv(t, k);
				fp_prime_back(k, t);
				neg = (bn_cmp(k, pm1o2) == RLC_LT ? 0 : 1);
				ep_sw_bn(q, t, -1, neg);
				ep_add(p, p, q);
				ep_norm(p, p);
				break;
			case EP_B12:
				ep_sw_b12(p, t, -3, neg);
				md_map(digest, digest, RLC_MD_LEN);
				bn_read_bin(k, digest, RLC_MIN(RLC_FP_BYTES, RLC_MD_LEN));
				fp_prime_conv(t, k);
				neg = (bn_cmp(k, pm1o2) == RLC_LT ? 0 : 1);
				ep_sw_b12(q, t, -3, neg);
				ep_add(p, p, q);
				ep_norm(p, p);
				/* Now, multiply by cofactor to get the correct group. */
				fp_prime_get_par(k);
				bn_neg(k, k);
				bn_add_dig(k, k, 1);
				if (bn_bits(k) < RLC_DIG) {
					ep_mul_dig(p, p, k->dp[0]);
				} else {
					ep_mul(p, p, k);
				}
				break;
			default:
				fp_prime_conv(p->x, k);
				fp_zero(p->y);
				fp_set_dig(p->z, 1);

				while (1) {
					ep_rhs(t, p);

					if (fp_srt(p->y, t)) {
						p->norm = 1;
						break;
					}
					fp_add_dig(p->x, p->x, 1);
				}

				/* Now, multiply by cofactor to get the correct group. */
				ep_curve_get_cof(k);
				if (bn_bits(k) < RLC_DIG) {
					ep_mul_dig(p, p, k->dp[0]);
				} else {
					ep_mul_basic(p, p, k);
				}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(k);
		bn_free(pm1o2);
		fp_free(t);
		ep_free(q);
	}
}
