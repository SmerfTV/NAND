#ifdef NDEBUG
#undef NDEBUG
#endif

#include "nand.h"
#include "memory_tests.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

/** MAKRA SKRACAJĄCE IMPLEMENTACJĘ TESTÓW **/

// To są możliwe wyniki testu.
#define PASS 0
#define FAIL 1
#define WRONG_TEST 2

// Oblicza liczbę elementów tablicy x.
#define SIZE(x) (sizeof x / sizeof x[0])

#define TEST_EINVAL(f)                  \
  do {                                  \
    errno = 0;                          \
    if ((f) != -1 || errno != EINVAL)   \
      return FAIL;                      \
  } while (0)

#define TEST_ECANCELED(f)               \
  do {                                  \
    errno = 0;                          \
    if ((f) != -1 || errno != ECANCELED)\
      return FAIL;                      \
  } while (0)

#define TEST_NULL_EINVAL(f)             \
  do {                                  \
    errno = 0;                          \
    if ((f) != NULL || errno != EINVAL) \
      return FAIL;                      \
  } while (0)

#define TEST_PASS(f)                    \
  do {                                  \
    if ((f) != 0)                       \
      return FAIL;                      \
  } while (0)

#define ASSERT(f)                       \
  do {                                  \
    if (!(f))                           \
      return FAIL;                      \
  } while (0)

#define V(code, where) (((unsigned long)code) << (3 * where))

/** WŁAŚCIWE TESTY **/

// Testuje poprawność weryfikacji parametrów wywołań funkcji.
static int params(void) {
  nand_t *g1 = nand_new(2);
  nand_t *g2 = nand_new(2);
  assert(g1);
  assert(g2);

  bool s[2];
  nand_t *g[2] = {g1, g2};

  TEST_EINVAL(nand_connect_nand(g1, NULL, 0));
  TEST_EINVAL(nand_connect_nand(NULL, g2, 0));
  TEST_EINVAL(nand_connect_nand(NULL, NULL, 0));
  TEST_EINVAL(nand_connect_nand(g1, g2, 2));
  TEST_EINVAL(nand_connect_nand(g1, g2, UINT_MAX));

  TEST_EINVAL(nand_connect_signal(NULL, g2, 1));
  TEST_EINVAL(nand_connect_signal(s, NULL, 1));
  TEST_EINVAL(nand_connect_signal(NULL, NULL, 1));
  TEST_EINVAL(nand_connect_signal(s, g2, 2));
  TEST_EINVAL(nand_connect_signal(s, g2, UINT_MAX));

  TEST_EINVAL(nand_evaluate(NULL, s, 2));
  TEST_EINVAL(nand_evaluate(g, NULL, 2));
  TEST_EINVAL(nand_evaluate(NULL, NULL, 2));
  TEST_EINVAL(nand_evaluate(g, s, 0));
  TEST_EINVAL(nand_evaluate(NULL, s, 0));
  TEST_EINVAL(nand_evaluate(g, NULL, 0));
  TEST_EINVAL(nand_evaluate(NULL, NULL, 0));

  g[1] = NULL;
  TEST_EINVAL(nand_evaluate(g, s, 2));
  g[0] = NULL;
  TEST_EINVAL(nand_evaluate(g, s, 2));

  TEST_EINVAL(nand_fan_out(NULL));

  TEST_NULL_EINVAL(nand_input(NULL, 0));
  TEST_NULL_EINVAL(nand_input(g1, 2));
  TEST_NULL_EINVAL(nand_input(g1, UINT_MAX));

  nand_delete(g1);
  nand_delete(g2);

  return PASS;
}

// To jest przykładowy test udostępniony studentom.
static int example(void) {
  nand_t *g[3];
  ssize_t path_len;
  bool s_in[2], s_out[3];

  g[0] = nand_new(2);
  g[1] = nand_new(2);
  g[2] = nand_new(2);
  assert(g[0]);
  assert(g[1]);
  assert(g[2]);

  TEST_PASS(nand_connect_nand(g[2], g[0], 0));
  TEST_PASS(nand_connect_nand(g[2], g[0], 1));
  TEST_PASS(nand_connect_nand(g[2], g[1], 0));

  TEST_PASS(nand_connect_signal(s_in + 0, g[2], 0));
  TEST_PASS(nand_connect_signal(s_in + 1, g[2], 1));
  TEST_PASS(nand_connect_signal(s_in + 1, g[1], 1));

  ASSERT(0 == nand_fan_out(g[0]));
  ASSERT(0 == nand_fan_out(g[1]));
  ASSERT(3 == nand_fan_out(g[2]));

  int c[3] = {0};
  for (ssize_t i = 0; i < 3; ++i) {
    nand_t *t = nand_output(g[2], i);
    for (int j = 0; j < 3; ++j)
      if (g[j] == t)
        c[j]++;
  }
  ASSERT(c[0] == 2 && c[1] == 1 && c[2] == 0);

  s_in[0] = false, s_in[1] = false;
  path_len = nand_evaluate(g, s_out, 3);
  ASSERT(path_len == 2 && s_out[0] == false && s_out[1] == true && s_out[2] == true);

  s_in[0] = true, s_in[1] = false;
  path_len = nand_evaluate(g, s_out, 3);
  ASSERT(path_len == 2 && s_out[0] == false && s_out[1] == true && s_out[2] == true);

  s_in[0] = false, s_in[1] = true;
  path_len = nand_evaluate(g, s_out, 3);
  ASSERT(path_len == 2 && s_out[0] == false && s_out[1] == false && s_out[2] == true);

  s_in[0] = true, s_in[1] = true;
  path_len = nand_evaluate(g, s_out, 3);
  ASSERT(path_len == 2 && s_out[0] == true && s_out[1] == true && s_out[2] == false);

  nand_delete(g[0]);
  nand_delete(g[1]);
  nand_delete(g[2]);

  return PASS;
}

// Testuje proste przypadki.
static int simple(void) {
  for (int i = 0; i < 3; ++i) {
    bool s[1];

    nand_t *g0 = nand_new(0);
    nand_t *g1 = nand_new(1);
    nand_t *g2 = nand_new(100);
    assert(g0);
    assert(g1);
    assert(g2);

    TEST_PASS(nand_connect_nand(g1, g1, 0));

    ASSERT(0 == nand_evaluate(&g0, s, 1));
    ASSERT(s[0] == false);

    TEST_ECANCELED(nand_evaluate(&g1, s, 1));
    TEST_ECANCELED(nand_evaluate(&g2, s, 1));

    nand_delete(NULL);
    nand_delete(g0);
    nand_delete(g1);
    nand_delete(g2);
  }

  return PASS;
}

// Testuje proste przypadki podłączania i odłączania sygnałów.
static int connect(void) {
  bool s[1];

  nand_t *g0 = nand_new(3);
  nand_t *g1 = nand_new(2);
  nand_t *g2 = nand_new(2);
  nand_t *g3 = nand_new(2);
  assert(g0);
  assert(g1);
  assert(g2);
  assert(g3);

  TEST_PASS(nand_connect_signal(s, g0, 1));
  ASSERT(nand_input(g0, 1) == (void*)s);
  TEST_PASS(nand_connect_nand(g1, g0, 1));
  ASSERT(nand_input(g0, 1) == (void*)g1);
  nand_delete(g1);
  ASSERT(nand_input(g0, 1) == NULL);

  ASSERT(nand_fan_out(g0) == 0);
  TEST_PASS(nand_connect_nand(g0, g2, 1));
  ASSERT(nand_fan_out(g0) == 1);
  ASSERT(nand_output(g0, 0) == g2);
  TEST_PASS(nand_connect_nand(g0, g3, 1));
  ASSERT(nand_fan_out(g0) == 2);
  nand_delete(g2);
  ASSERT(nand_fan_out(g0) == 1);
  ASSERT(nand_output(g0, 0) == g3);
  nand_delete(g3);
  ASSERT(nand_fan_out(g0) == 0);

  nand_delete(g0);

  return PASS;
}

// Testuje poprawność wyliczania wartości na przykładzie sumatora szeregowego.
static int adder(void) {
  static const unsigned BITS = 4;

  nand_t *g[BITS][12], *y[BITS + 1];
  bool a[BITS], b[BITS], c[1], s[BITS + 1];

  for (unsigned i = 0; i < BITS; ++i) {
    g[i][0] = nand_new(1);
    g[i][1] = nand_new(1);
    g[i][2] = nand_new(1);
    g[i][3] = nand_new(2);
    g[i][4] = nand_new(2);
    g[i][5] = nand_new(2);
    g[i][6] = nand_new(3);
    g[i][7] = nand_new(3);
    g[i][8] = nand_new(3);
    g[i][9] = nand_new(3);
    g[i][10] = nand_new(3);
    g[i][11] = nand_new(4);
    for (unsigned j = 0; j < 12; ++j)
      assert(g[i][j]);
  }

  for (unsigned i = 0; i < BITS; ++i) {
    TEST_PASS(nand_connect_signal(&a[i], g[i][1], 0));
    TEST_PASS(nand_connect_signal(&a[i], g[i][3], 0));
    TEST_PASS(nand_connect_signal(&a[i], g[i][4], 0));
    TEST_PASS(nand_connect_signal(&a[i], g[i][6], 0));
    TEST_PASS(nand_connect_signal(&a[i], g[i][7], 0));
    TEST_PASS(nand_connect_signal(&b[i], g[i][2], 0));
    TEST_PASS(nand_connect_signal(&b[i], g[i][3], 1));
    TEST_PASS(nand_connect_signal(&b[i], g[i][5], 1));
    TEST_PASS(nand_connect_signal(&b[i], g[i][7], 1));
    TEST_PASS(nand_connect_signal(&b[i], g[i][8], 1));
    TEST_PASS(nand_connect_nand(g[i][0], g[i][6], 2));
    TEST_PASS(nand_connect_nand(g[i][0], g[i][8], 2));
    TEST_PASS(nand_connect_nand(g[i][1], g[i][8], 0));
    TEST_PASS(nand_connect_nand(g[i][1], g[i][9], 0));
    TEST_PASS(nand_connect_nand(g[i][2], g[i][6], 1));
    TEST_PASS(nand_connect_nand(g[i][2], g[i][9], 1));
    TEST_PASS(nand_connect_nand(g[i][3], g[i][10], 0));
    TEST_PASS(nand_connect_nand(g[i][4], g[i][10], 1));
    TEST_PASS(nand_connect_nand(g[i][5], g[i][10], 2));
    TEST_PASS(nand_connect_nand(g[i][6], g[i][11], 3));
    TEST_PASS(nand_connect_nand(g[i][7], g[i][11], 2));
    TEST_PASS(nand_connect_nand(g[i][8], g[i][11], 1));
    TEST_PASS(nand_connect_nand(g[i][9], g[i][11], 0));
    if (i == 0) {
      TEST_PASS(nand_connect_signal(c, g[i][0], 0));
      TEST_PASS(nand_connect_signal(c, g[i][4], 1));
      TEST_PASS(nand_connect_signal(c, g[i][5], 0));
      TEST_PASS(nand_connect_signal(c, g[i][7], 2));
      TEST_PASS(nand_connect_signal(c, g[i][9], 2));
    }
    else {
      TEST_PASS(nand_connect_nand(g[i - 1][10], g[i][0], 0));
      TEST_PASS(nand_connect_nand(g[i - 1][10], g[i][4], 1));
      TEST_PASS(nand_connect_nand(g[i - 1][10], g[i][5], 0));
      TEST_PASS(nand_connect_nand(g[i - 1][10], g[i][7], 2));
      TEST_PASS(nand_connect_nand(g[i - 1][10], g[i][9], 2));
    }
    y[i] = g[i][11];
  }
  y[BITS] = g[BITS - 1][10];

  for (unsigned i = 0; i < (1 << BITS); ++i) {
    for (unsigned n = 0; n < BITS; ++n)
      a[n] = (bool)((i >> n) & 1);
    for (unsigned j = 0; j < (1 << BITS); ++j) {
      for (unsigned n = 0; n < BITS; ++n)
        b[n] = (bool)((j >> n) & 1);
      for (unsigned k = 0; k < 2; ++k) {
        c[0] = (bool)k;
        ASSERT(nand_evaluate(y, s, BITS + 1) == 2 * BITS + 1);
        unsigned w = 0;
        for (unsigned n = 0; n <= BITS; ++n)
          w |= (unsigned)s[n] << n;
        ASSERT(w == i + j + k);
      }
    }
  }

  for (unsigned i = 0; i < BITS; ++i)
    for (unsigned j = 0; j < 12; ++j)
      nand_delete(g[i][j]);

  return PASS;
}

// Testuje, czy wyznaczenie wartości sygnału i długości ścieżki krytycznej na
// wyjściu bramki jest wykonywane tylko raz.
static int complexity(void) {
  static const int X = 157;
  static const int Y = 183;
  static const int Z = 139;

  nand_t *g0, *g1[Z], *g2[Y][X];
  bool s[X];

  g0 = nand_new(0);
  assert(g0);
  for (int i = 0; i < Z; ++i) {
    g1[i] = nand_new(1);
    assert(g1[i]);
  }
  for (int j = 0; j < Y; ++j) {
    for (int i = 0; i < X; ++i) {
      g2[j][i] = nand_new(X);
      assert(g2[j][i]);
    }
  }

  for (int j = 0; j < Y - 1; ++j)
    for (int i = 0; i < X; ++i)
      for (int k = 0; k < X; ++k)
        TEST_PASS(nand_connect_nand(g2[j + 1][k], g2[j][i], k));
  for (int i = 0; i < X; ++i)
    for (int k = 0; k < X; ++k)
      TEST_PASS(nand_connect_nand(g0, g2[Y - 1][i], k));
  for (int i = 0; i < Z - 1; ++i)
    TEST_PASS(nand_connect_nand(g1[i + 1], g1[i], 0));
  TEST_PASS(nand_connect_nand(g0, g1[Z - 1], 0));
  TEST_PASS(nand_connect_nand(g1[0], g2[Y - 1][X / 2], X / 2));

  ASSERT(nand_evaluate(g2[0], s, X) == Y + Z);
  for (int i = 0; i < X; ++i)
    ASSERT(s[i] == (bool)(Y & 1));

  nand_delete(g0);
  for (int i = 0; i < Z; ++i)
    nand_delete(g1[i]);
  for (int j = 0; j < Y; ++j)
    for (int i = 0; i < X; ++i)
      nand_delete(g2[j][i]);

  return PASS;
}

// Testuje bramkę o dużej liczbie wejść.
static int input(void) {
  static const unsigned FAN_IN = 1000;

  bool s_in[FAN_IN], s_out;

  nand_t *g = nand_new(FAN_IN);
  assert(g);

  for (unsigned i = 0; i < FAN_IN; ++i) {
    s_in[i] = true;
    TEST_PASS(nand_connect_signal(s_in + i, g, i));
  }

  ASSERT(nand_evaluate(&g, &s_out, 1) == 1);
  ASSERT(s_out == false);

  for (unsigned i = 0; i < FAN_IN; ++i) {
    s_in[i] = false;
    ASSERT(nand_evaluate(&g, &s_out, 1) == 1);
    ASSERT(s_out == true);
    s_in[i] = true;
    ASSERT(nand_input(g, i) == s_in + i);
  }

  nand_delete(g);

  return PASS;
}

// Testuje bramkę z dużą liczbą bramek podłączonych do jej wyjścia.
static int output(void) {
  static const ssize_t FAN_OUT = 45000;

  nand_t *g[FAN_OUT], *h;
  uint64_t x = 0, y = 0;

  for (ssize_t i = 0; i < FAN_OUT; ++i) {
    g[i] = nand_new(1);
    assert(g[i]);
    x ^= (uint64_t)g[i];
  }

  h = nand_new(2);
  assert(h);

  for (ssize_t i = 0; i < FAN_OUT; ++i)
    TEST_PASS(nand_connect_nand(h, g[i], 0));

  ASSERT(nand_fan_out(h) == FAN_OUT);

  for (ssize_t i = FAN_OUT - 1; i >= 0; --i)
    y ^= (uint64_t)nand_output(h, i);

  ASSERT(x == y);

  for (ssize_t i = 0; i < FAN_OUT; ++i)
    nand_delete(g[i]);
  nand_delete(h);

  return PASS;
}

// Testuje jedną bramkę przy obecności wielu innych bramek ułożonych w linię.
static int line(void) {
  static const int GATES = 10000;
  static const int TEST_COUNT = 10000;

  nand_t *g[GATES];
  bool s_in, s_out;

  g[0] = nand_new(1);
  assert(g[0]);
  TEST_PASS(nand_connect_signal(&s_in, g[0], 0));

  for (int i = 1; i < GATES; ++i) {
    g[i] = nand_new(1);
    assert(g[i]);
    TEST_PASS(nand_connect_nand(g[i - 1], g[i], 0));
  }

  for (int j = 0; j < TEST_COUNT; ++j) {
    s_in = j & 1;
    ASSERT(nand_evaluate(g, &s_out, 1) == 1);
    ASSERT(s_out == !s_in);
  }

  for (int i = 0; i < GATES; ++i)
    nand_delete(g[i]);

  return PASS;
}

// Testuje ciąg bramek ułożonych w okrąg.
static int circle(void) {
  static const int GATES = 128;
  static const int t[] = {57, 57, 58, 56, 57, 85};

  nand_t *g[GATES];
  bool s_in, s_out;

  for (int i = 0; i < GATES; ++i) {
    g[i] = nand_new(1);
    assert(g[i]);
  }

  for (int i = 1; i < GATES; ++i)
    TEST_PASS(nand_connect_nand(g[i - 1], g[i], 0));
  TEST_PASS(nand_connect_nand(g[GATES - 1], g[0], 0));

  for (size_t k = 0; k < SIZE(t); ++k) {
    int j = t[k];

    TEST_ECANCELED(nand_evaluate(&g[j], &s_out, 1));
    nand_delete(g[j]);
    TEST_ECANCELED(nand_evaluate(&g[j - 1], &s_out, 1));
    TEST_PASS(nand_connect_signal(&s_in, g[j + 1], 0));

    s_in = true;
    ASSERT(nand_evaluate(&g[j - 1], &s_out, 1) ==  GATES - 1);
    ASSERT(s_out == (GATES & 1));

    s_in = false;
    ASSERT(nand_evaluate(&g[j - 1], &s_out, 1) ==  GATES - 1);
    ASSERT(s_out == !(GATES & 1));

    g[j] = nand_new(1);
    assert(g[j]);
    TEST_PASS(nand_connect_nand(g[j - 1], g[j], 0));
    TEST_PASS(nand_connect_nand(g[j], g[j + 1], 0));
  }

  for (int i = 0; i < GATES; ++i)
    nand_delete(g[i]);

  return PASS;
}

// Testuje programowalną macierz bramek.
static int pla(void) {
  #define K 4
  #define L 5
  #define M 3
  #define P 3
  #define N 2

  static const bool romx0[P][L][K] =
  {
    {
      {1, 0, 1, 0},
      {0, 1, 0, 0},
      {0, 0, 1, 0},
      {1, 1, 0, 0},
      {0, 0, 1, 1},
    },
    {
      {0, 1, 0, 0},
      {0, 1, 1, 0},
      {0, 1, 0, 1},
      {1, 0, 1, 0},
      {1, 0, 0, 0},
    },
    {
      {0, 0, 0, 1},
      {0, 0, 1, 0},
      {0, 1, 0, 0},
      {1, 0, 0, 0},
      {0, 1, 1, 1},
    },
  };
  static const bool romx1[P][L][K] =
  {
    {
      {0, 1, 1, 0},
      {0, 0, 0, 0},
      {0, 1, 0, 0},
      {0, 0, 0, 1},
      {1, 0, 0, 0},
    },
    {
      {1, 0, 0, 0},
      {0, 1, 0, 0},
      {0, 0, 0, 0},
      {0, 0, 1, 0},
      {0, 0, 0, 1},
    },
    {
      {0, 0, 0, 0},
      {0, 1, 1, 0},
      {0, 1, 1, 0},
      {0, 1, 1, 0},
      {0, 0, 0, 0},
    },
  };
  static const bool romy[P][M][L] =
  {
    {
      {0, 0, 1, 0, 0},
      {0, 1, 0, 1, 0},
      {1, 0, 0, 0, 1},
    },
    {
      {1, 0, 0, 0, 1},
      {0, 1, 0, 1, 0},
      {0, 0, 1, 0, 0},
    },
    {
      {0, 1, 0, 1, 1},
      {1, 0, 0, 1, 0},
      {0, 0, 1, 0, 0},
    },
  };
  static const bool romz[P][M] =
  {
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1},
  };
  static const bool in_data[P][N][K] =
  {
    {
      {0, 1, 0, 1},
      {1, 0, 1, 0},
    },
    {
      {0, 0, 1, 1},
      {1, 1, 0, 0},
    },
    {
      {1, 0, 1, 0},
      {0, 1, 0, 1},
    },
  };
  static const bool out_expected[P][N][M] =
  {
    {
      {1, 1, 0},
      {0, 0, 1},
    },
    {
      {0, 0, 1},
      {1, 0, 1},
    },
    {
      {1, 0, 1},
      {0, 1, 0},
    },
  };
  static const ssize_t path[P][N] = {
    {4, 4},
    {4, 4},
    {4, 4},
  };

  nand_t *in_not[K], *and[L], *or[M], *out_not[M], *out[M];
  bool one = true, inputs[K], outputs[M];

  for (int i = 0; i < K; ++i) {
    in_not[i] = nand_new(1);
    assert(in_not[i]);
  }
  for (int i = 0; i < L; ++i) {
    and[i] = nand_new(K);
    assert(and[i]);
  }
  for (int i = 0; i < M; ++i) {
    or[i] = nand_new(L);
    assert(or[i]);
    out_not[i] = nand_new(1);
    assert(out_not[i]);
  }

  for (int i = 0; i < K; ++i)
    TEST_PASS(nand_connect_signal(inputs + i, in_not[i], 0));
  for (int i = 0; i < M; ++i)
    TEST_PASS(nand_connect_nand(or[i], out_not[i], 0));

  for (int p = 0; p < P; ++p) {
    for (int l = 0; l < L; ++l)
      for (int k = 0; k < K; ++k)
        if (romx0[p][l][k])
          TEST_PASS(nand_connect_nand(in_not[k], and[l], k));
        else if (romx1[p][l][k])
          TEST_PASS(nand_connect_signal(inputs + k, and[l], k));
        else
          TEST_PASS(nand_connect_signal(&one, and[l], k));
    for (int m = 0; m < M; ++m)
      for (int l = 0; l < L; ++l)
        if (romy[p][m][l])
          TEST_PASS(nand_connect_nand(and[l], or[m], l));
        else
          TEST_PASS(nand_connect_signal(&one, or[m], l));
    for (int m = 0; m < M; ++m)
      if (romz[p][m])
        out[m] = or[m];
      else
        out[m] = out_not[m];
    for (int n = 0; n < N; ++n) {
      for (int k = 0; k < K; ++k)
        inputs[k] = in_data[p][n][k];
      // printf("%zd\n", nand_evaluate(out, outputs, M));
      ASSERT(nand_evaluate(out, outputs, M) == path[p][n]);
      // for (int m = 0; m < M; ++m)
      //   printf("%d\n", outputs[m]);
      for (int m = 0; m < M; ++m)
        ASSERT(outputs[m] == out_expected[p][n][m]);
    }
  }

  for (int i = 0; i < K; ++i)
    nand_delete(in_not[i]);
  for (int i = 0; i < L; ++i)
    nand_delete(and[i]);
  for (int i = 0; i < M; ++i)
    nand_delete(or[i]);
  for (int i = 0; i < M; ++i)
    nand_delete(out_not[i]);

  return PASS;

  #undef K
  #undef L
  #undef M
  #undef P
  #undef N
}

// Testuje prosty układ z pętlą.
static int flipflop(void) {
  nand_t *g[2];
  bool rs[2], q[2];

  g[0] = nand_new(2);
  g[1] = nand_new(2);
  assert(g[0]);
  assert(g[1]);

  TEST_PASS(nand_connect_nand(g[0], g[1], 0));
  TEST_PASS(nand_connect_nand(g[1], g[0], 1));
  TEST_PASS(nand_connect_signal(&rs[0], g[0], 0));
  TEST_PASS(nand_connect_signal(&rs[1], g[1], 1));

  rs[0] = false;
  rs[1] = false;
  TEST_ECANCELED(nand_evaluate(g, q, 2));

  nand_delete(g[0]);
  nand_delete(g[1]);

  return PASS;
}

// Testuje dwie bramki not.
static int notnot(void) {
  #define K0 500
  #define K1 400

  nand_t *g[2];
  bool in, out;

  g[0] = nand_new(K0);
  g[1] = nand_new(K1);
  assert(g[0]);
  assert(g[1]);

  for (unsigned k = 0; k < K0; ++k)
    TEST_PASS(nand_connect_signal(&in, g[0], k));
  for (unsigned k = 0; k < K1; ++k)
    TEST_PASS(nand_connect_nand(g[0], g[1], k));

  in = false;
  ASSERT(nand_evaluate(&g[1], &out, 1) == 2);
  ASSERT(out == false);

  in = true;
  ASSERT(nand_evaluate(&g[1], &out, 1) == 2);
  ASSERT(out == true);

  ASSERT(nand_fan_out(g[0]) == K1);
  ASSERT(nand_fan_out(g[1]) == 0);

  for (ssize_t k = 0; k < K1; ++k)
    ASSERT(nand_output(g[0], k) == g[1]);

  for (unsigned k = 0; k < K0; ++k)
    ASSERT(nand_input(g[0], k) == &in);

  for (unsigned k = 0; k < K1; ++k)
    ASSERT(nand_input(g[1], k) == g[0]);

  nand_delete(g[0]);
  nand_delete(g[1]);

  return PASS;

  #undef K0
  #undef K1
}

// Testuje reakcję implementacji na niepowodzenie alokacji pamięci.
static unsigned long alloc_fail_test(void) {
  unsigned long visited = 0;
  nand_t *nand1, *nand2;
  ssize_t len;
  int result;
  bool s_in[2], s_out[1];

  errno = 0;
  if ((nand1 = nand_new(2)) != NULL)
    visited |= V(1, 0);
  else if (errno == ENOMEM && (nand1 = nand_new(2)) != NULL)
    visited |= V(2, 0);
  else
    return visited |= V(4, 0);

  errno = 0;
  if ((nand2 = nand_new(1)) != NULL)
    visited |= V(1, 1);
  else if (errno == ENOMEM && (nand2 = nand_new(1)) != NULL)
    visited |= V(2, 1);
  else
    return visited |= V(4, 1);

  errno = 0;
  if ((result = nand_connect_nand(nand2, nand1, 0)) == 0)
    visited |= V(1, 2);
  else if (result == -1 && errno == ENOMEM && nand_connect_nand(nand2, nand1, 0) == 0)
    visited |= V(2, 2);
  else
    return visited |= V(4, 2);

  errno = 0;
  if ((result = nand_connect_signal(&s_in[1], nand1, 1)) == 0)
    visited |= V(1, 3);
  else if (result == -1 && errno == ENOMEM && nand_connect_signal(&s_in[1], nand1, 1) == 0)
    visited |= V(2, 3);
  else
    return visited |= V(4, 3);

  errno = 0;
  if ((result = nand_connect_signal(&s_in[0], nand2, 0)) == 0)
    visited |= V(1, 4);
  else if (result == -1 && errno == ENOMEM && nand_connect_signal(&s_in[0], nand2, 0) == 0)
    visited |= V(2, 4);
  else
    return visited |= V(4, 4);

  s_in[0] = false;
  s_in[1] = false;
  errno = 0;
  if ((len = nand_evaluate(&nand1, s_out, 1)) == 2 && s_out[0] == true)
    visited |= V(1, 5);
  else if (len == -1 && (errno == ENOMEM || errno == ECANCELED) && nand_evaluate(&nand1, s_out, 1) == 2 && s_out[0] == true)
    visited |= V(2, 5);
  else
    return visited |= V(4, 5);

  s_in[0] = false;
  s_in[1] = true;
  errno = 0;
  if ((len = nand_evaluate(&nand1, s_out, 1)) == 2 && s_out[0] == false)
    visited |= V(1, 6);
  else if (len == -1 && (errno == ENOMEM || errno == ECANCELED) && nand_evaluate(&nand1, s_out, 1) == 2 && s_out[0] == false)
    visited |= V(2, 6);
  else
    return visited |= V(4, 6);

  nand_delete(nand2);

  errno = 0;
  if ((result = nand_connect_signal(&s_in[0], nand1, 0)) == 0)
    visited |= V(1, 7);
  else if (result == -1 && errno == ENOMEM && nand_connect_signal(&s_in[0], nand1, 0) == 0)
    visited |= V(2, 7);
  else
    return visited |= V(4, 7);

  s_in[0] = true;
  s_in[1] = true;
  errno = 0;
  if ((len = nand_evaluate(&nand1, s_out, 1)) == 1 && s_out[0] == false)
    visited |= V(1, 8);
  else if (len == -1 && (errno == ENOMEM || errno == ECANCELED) && nand_evaluate(&nand1, s_out, 1) == 1 && s_out[0] == false)
    visited |= V(2, 8);
  else
    return visited |= V(4, 8);

  nand_delete(nand1);

  return visited;
}

// Testuje reakcję implementacji na niepowodzenie alokacji pamięci.
static int memory_test(unsigned long (* test_function)(void)) {
  memory_test_data_t *mtd = get_memory_test_data();

  unsigned fail = 0, pass = 0;
  mtd->call_total = 0;
  mtd->fail_counter = 1;
  while (fail < 3 && pass < 3) {
    mtd->call_counter = 0;
    mtd->alloc_counter = 0;
    mtd->free_counter = 0;
    mtd->function_name = NULL;
    unsigned long visited_points = test_function();
    if (mtd->alloc_counter != mtd->free_counter ||
        (visited_points & 0444444444444444444444UL) != 0) {
      fprintf(stderr,
              "fail_counter %u, alloc_counter %u, free_counter %u, "
              "function_name %s, visited_point %lo\n",
              mtd->fail_counter, mtd->alloc_counter, mtd->free_counter,
              mtd->function_name, visited_points);
      ++fail;
    }
    if (mtd->function_name == NULL)
      ++pass;
    else
      pass = 0;
    mtd->fail_counter++;
  }

  return mtd->call_total > 0 && fail == 0 ? PASS : FAIL;
}

// Testuje reakcję implementacji na niepowodzenie alokacji pamięci.
static int memory(void) {
  memory_tests_check();
  return memory_test(alloc_fail_test);
}

/** URUCHAMIANIE TESTÓW **/

typedef struct {
  char const *name;
  int (*function)(void);
} test_list_t;

#define TEST(t) {#t, t}

static const test_list_t test_list[] = {
  TEST(params),
  TEST(example),
  TEST(simple),
  TEST(connect),
  TEST(adder),
  TEST(complexity),
  TEST(input),
  TEST(output),
  TEST(line),
  TEST(circle),
  TEST(pla),
  TEST(flipflop),
  TEST(notnot),
  TEST(memory),
};

static int do_test(int (*function)(void)) {
  int result = function();
  puts("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
  return result;
}

int main(int argc, char *argv[]) {
  if (argc == 2)
    for (size_t i = 0; i < SIZE(test_list); ++i)
      if (strcmp(argv[1], test_list[i].name) == 0)
        return do_test(test_list[i].function);

  fprintf(stderr, "Użycie:\n%s nazwa_testu\n", argv[0]);
  return WRONG_TEST;
}
