// head
#ifndef MOPP
// ok
#endif

#ifndef MOPP
// ok
#else
// error
#endif

#ifdef MOPP
// error
#endif

#ifdef MOPP
// error
#else
// ok
#endif

#define MOPP

#ifndef MOPP
// error
#endif

#ifndef MOPP
// error
#else
// ok
#endif

#ifdef MOPP
// ok
#endif

#ifdef MOPP
// ok
#else
// error
#endif

int main(void) {
    return NULL;
}
