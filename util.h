#define CAT(A, B) A##B
#define PADNAME(X) CAT(pad, X)
#define PAD(X) char PADNAME(__LINE__)[X];

#define DEG2RAD(X) ((X) * M_PI / 180.0)
#define RAD2DEG(X) ((X) * 180.0 / M_PI)
