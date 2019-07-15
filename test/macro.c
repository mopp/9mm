// head
#ifndef MOPP
// ok ifndef 1
#else
// error 1
#endif
#ifdef MOPP
// error 2
#endif
#ifdef MOPP
// error 3
#else
// ok ifdef 2
#endif
#define MOPP
#ifndef MOPP
// error 4
#endif
#ifndef MOPP
// error 5
#else
// ok ifndef 3
#endif
#ifdef MOPP
// ok ifdef 4
#endif
#ifdef MOPP
// ok ifdef 5
#else
// error 6
#endif
// tail

int main(void) {
    return NULL;
}
