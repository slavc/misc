#include <stdio.h>
#include <stdlib.h>
#include <openssl/bn.h>

static void
usage(void)
{
	printf("usage: crackrsa <num_bits> <e> <n>\n");
	printf("Look for primes p and q whose product is n.\n");
	printf("n must be specified in decimal format.\n");
}

int
main(int argc, char **argv)
{
	if (argc < 4) {
		usage();
		return 1;
	}

	int num_bits = atoi(argv[1]);

	BIGNUM *e = NULL;
	BN_dec2bn(&e, argv[2]);

	BIGNUM *n = NULL;
	BN_dec2bn(&n, argv[3]);

	BN_CTX *ctx = NULL;
	ctx = BN_CTX_new();
	if (ctx == NULL)
		return 2;
	BN_CTX_start(ctx);

	BIGNUM *r0 = NULL, *r1 = NULL, *r2 = NULL, *r3 = NULL;
	r0 = BN_CTX_get(ctx);
	r1 = BN_CTX_get(ctx);
	r2 = BN_CTX_get(ctx);
	r3 = BN_CTX_get(ctx);

	BIGNUM *p = NULL; //BN_new();
	BIGNUM *quot = BN_new();
	BIGNUM *rem = BN_new();
	for (;;) {
		//BN_generate_prime_ex(p, num_bits, 0, NULL, NULL, NULL);
		BN_dec2bn(&p, argv[4]);

		BN_sub(r2, p, BN_value_one());
		BN_gcd(r1, r2, rsa->e, ctx);
		if (!BN_is_one(r1))
			continue;
		BN_div(quot, rem, n, p, ctx);
		if (BN_is_zero(rem))
			break;
	}

	char *p_str = BN_bn2dec(p);
	char *q_str = BN_bn2dec(quot);

	printf("p=%s\n", p_str);
	printf("q=%s\n", q_str);

	return 0;
}
