/*
* DiNo Release 1.0
* Copyright (C) 2015: W. Piotrowski, M. Fox, D. Long, D. Magazzeni, F. Mercorio
*
* Read the file "LICENSE.txt" distributed with these sources, or call
* DiNo with the -l switch for additional information.
* Contact: (wiktor.piotrowski@kcl.ac.uk)
*
*/

#include "upm_statecl.hpp"
#include <limits.h>
#include <vector>

// if you change this,
// please make sure that you change a lot of other things
// including array index error checking
#ifndef ALIGN
// Norris: Must be zero
#define UNDEFVAL 0
#define UNDEFVALLONG 0
#else
// Norris: the biggest value storable
#define UNDEFVAL 0xff
#define UNDEFVALLONG INT_MIN
#endif

/****************************************
  There are 4 groups of declarations:
  1) utility functions (not in any class)
  2) mu__int / mu__boolean
  3) world_class
  4) timer (not in any class)
  5) random number generator
 ****************************************/

/****************************************
  Utility functions
 ****************************************/
const char *StrStr(const char *super, const char *sub);
void ErrAlloc(void *p);
void err_new_handler(void);

// Uli: OLD_CATCH_DIV has to be defined or undefined depending on compiler
//      and operating system (see Makefile)
#ifdef OLD_CATCH_DIV
void catch_div_by_zero(...);	// for most compilers
#else
void catch_div_by_zero(int);	// for CC on elaines
#endif

bool IsPrime(unsigned long n);
unsigned long NextPrime(unsigned long n);
unsigned long NumStatesGivenBytes(unsigned long bytes);
char *tsprintf(const char *fmt, ...);



/*
 * WP WP WP WP WP WP WP WP WP WP WP WP WP WP WP WP
 *
 * CREATED A SUPERCLASS TO ENCAPSULATE MU_1_REAL_TYPE AND MU_0_BOOLEAN
 *
 * FOR USE IN RPG
 *
 */

class mu__any{

protected:
	mu__any(){}

public:
	  virtual int operator=(int val) {}
	  virtual char *get_mu_type(){}
	  virtual ~mu__any()
	  {
	    // Compulsory virtual destructor definition,
	    // even if it's empty
	  }

	  virtual void print(FILE *target=stdout, const char *separator="\n"){}

};





/****************************************
  The base class for a value
 ****************************************/

// Uli: general comments about the variables:
// - they are local by default
// - they are made global by calling to_state()

class mu__int: public mu__any			/* a base for a value. */ // WP WP WP WP WP
{
  enum { undef_value = UNDEFVAL };	// Uli: nice way to declare constants

  void operator=(mu__int &);	// Uli: disallow copying, this can catch
  //      possible errors in the Murphi
  //      compiler

 protected:

  bool in_world;		// if TRUE global variable, otherwise local
  int lb, ub;			// lower bound and upper bound
  int offset, size;		// offset in the state vector, size (both in bits)
#ifndef ALIGN
  position where;		// more info on position in state vector
#else
  int byteOffset;		// offset in state vector in bytes
  unsigned char *valptr;	// points either into state vector (for globals)
  //   or to data member cvalue
#endif
  bool initialized;		// whether it is initialized in the startstate
  //   Uli: seems not to be used any more
  unsigned char cvalue;		// contains value of local variable

 public:
  char *name;			// name of the variable
  char longname[BUFFER_SIZE / 4];

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // constructors

  mu__int(int lb, int ub, int size, const char *n, int os)
    :lb(lb), ub(ub), size(size), initialized(FALSE)
  {
    set_self(n, os);
    undefined();
  };
  mu__int(int lb, int ub, int size)
    :lb(lb), ub(ub), size(size), initialized(FALSE)
  {
    set_self(NULL, 0);
    undefined();
  };

  // Uli: - seems not to be used any more
  //      - however, I left it here since it does the correct things
  mu__int(int lb, int ub, int size, int val)
    :lb(lb), ub(ub), size(size)
  {
    set_self("Parameter or function result", 0);
    operator=(val);
  };

  // Uli: this constructor is called implicitly for function results
  //      by the code generated by the Murphi compiler
  mu__int(const mu__int & src)
    :lb(src.lb), ub(src.ub), size(src.size), in_world(FALSE)
  {
    set_self("Function result", 0);
    value(src.value());		// value() allows returning undefined values
  };

  // a destructor.
  virtual ~ mu__int(void)
  {
  };

  // routines for constructor use
  void set_self_ar(const char *n1, const char *n2, int os)  	// sets the name
  {
    if (n1 == NULL) {
      set_self(NULL, 0);
      return;
    }
    int l1 = strlen(n1), l2 = strlen(n2);	// without using
    strcpy(longname, n1);	// tsprintf
    longname[l1] = '[';
    strcpy(longname + l1 + 1, n2);
    longname[l1 + l2 + 1] = ']';
    longname[l1 + l2 + 2] = 0;
    set_self(longname, os);
  };
  void set_self_2(const char *n1, const char *n2, int os)  	// sets the name
  {
    if (n1 == NULL) {
      set_self(NULL, 0);
      return;
    }
    strcpy(longname, n1);	// without using
    strcat(longname, n2);	// tsprintf
    set_self(longname, os);
  };
  void set_self(const char *n, int os)
  {
    /*if (n != NULL) {
      name = new char[strlen(n) + 1];
      strcpy(name, n);
    } else
      name = NULL;*/
    name = (char *)n;
    offset = os;
    in_world = FALSE;		// Uli: variables are local by default
#ifdef ALIGN
    byteOffset = offset / 8;
    valptr = (unsigned char *) &cvalue;
#else
    where.longoffset = os / 32;
    if (size < 0) {
      where.mask1 = 0;
      where.shift1 = 0;
      where.mask2 = 0;
      where.shift2 = 0;
    } else {
      where.shift1 = os % 32;
      where.mask1 = (1 << size) - 1;
      where.mask1 <<= where.shift1;
      where.mask2 = 0;
      if ((os + size) / 32 != where.longoffset) {
        where.shift2 = 32 - where.shift1;
        where.mask2 = (1 << (size - where.shift2)) - 1;
      }
    }
    where.longoffset *= 4;
#endif
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // data access routines

  // Uli: - the following three functions should be replaced by undefined(),
  //        etc.
  //      - left them here since they are called by the the code generated
  //        by the Murphi compiler
  inline void clear()		// set value to minimum
  {
    value(lb);
    initialized = TRUE;
  };
  inline void undefine()	// make variable be undefined
  {
    undefined();
    initialized = TRUE;
  };
  inline void reset()		// make variable be uninitialized
  {
    undefined();
    initialized = FALSE;
  };

  // assignment
#ifndef NO_RUN_TIME_CHECKING
  void boundary_error(int val) const;

  int operator=(int val)
  {
    if ((val <= ub) && (val >= lb)) {
      value(val);
      initialized = TRUE;
    } else
      boundary_error(val);
    return val;
  }
#else
  int operator=(int val)
  {
    return value(val);
  }
#endif

  // conversion to int
#ifndef NO_RUN_TIME_CHECKING
  int undef_error() const;

  inline operator   int () const
  {
    if (isundefined())
      return undef_error();
    return value();
  };
#else
  inline operator   int () const
  {
    return value();
  };
#endif

  // new data access routines
  // Uli: - the number of functions defined here could be reduced
  //      - new names would be good as well
  //        ("undefine" instead of "undefined", etc.)
#ifdef ALIGN
  inline const int value() const
  {
    return *valptr;
  };
  inline int value(int val)
  {
    *valptr = val;
    return val;
  };
  inline void defined(bool val)
  {
    if (!val)
      *valptr = undef_value;
  };
  inline bool defined() const
  {
    return (*valptr != undef_value);
  };
  inline void undefined()
  {
    *valptr = undef_value;
  };
  inline bool isundefined() const
  {
    return (*valptr == undef_value);
  };
#else
  inline const int value() const
  {
    if (in_world)
      return workingstate->get(&where) + lb - 1;
    else
      return cvalue + lb - 1;
  };
  inline int value(int val)
  {
    if (in_world)
      workingstate->set(&where, val - lb + 1);
    else
      cvalue = val - lb + 1;
    return val;
  };
  inline void defined(bool val)
  {
    if (!val)
      if (in_world)
        workingstate->set(&where, undef_value);
      else
        cvalue = undef_value;
  };
  inline bool defined() const
  {
    if (in_world)
      return (workingstate->get(&where) != undef_value);
    else
      return cvalue != undef_value;
  };
  inline void undefined()
  {
    defined(FALSE);
  };
  inline bool isundefined() const
  {
    if (in_world)
      return (workingstate->get(&where) == undef_value);
    else
      return cvalue == undef_value;
  };
#endif

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // printing, etc.

  // printing routines
  virtual void print(FILE *target=stdout, const char *separator="\n")
  {
    if (defined())
      fprintf(target,"%s: %d%s",name,value(),separator);
    //cout << name << ":" << value() << '\n';
    else
      fprintf(target,"%s: Undefined%s",name,separator);
    //cout << name << ":Undefined\n";
  };
  friend ostream & operator<<(ostream & s, mu__int & val)
  {
    if (val.defined())
      s << val.value();
    else
      s << "Undefined";
    return s;			// changed by Uli
  }
  void print_diff(state * prevstate, FILE *target, const char *separator);

  // transfer routines
  void to_state(state * thestate)
  {
    int val = value();		// Uli: copy value (which is probably undefined)
    in_world = TRUE;
#ifdef ALIGN
    valptr = (unsigned char *) &(workingstate->bits[byteOffset]);
#endif
    value(val);
  };
  void from_state(state * thestate)
  {
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // comparing routines, for symmetry

  friend int CompareWeight(mu__int & a, mu__int & b)
  {
    if (!a.defined() && !b.defined())
      return 0;
    else if (!a.defined())
      return -1;
    else if (!b.defined())
      return 1;
    else if (a.value() == b.value())
      return 0;
    else if (a.value() > b.value())
      return 1;
    else
      return -1;
  };

  friend int Compare(mu__int & a, mu__int & b)
  {
    if (!a.defined() && !b.defined())
      return 0;
    else if (!a.defined())
      return -1;
    else if (!b.defined())
      return 1;
    else if (a.value() == b.value())
      return 0;
    else if (a.value() > b.value())
      return 1;
    else
      return -1;
  };

  virtual void MultisetSort()
  {
  };

};



/****************************************
  The class for a short int (byte)
 ****************************************/

class mu__byte:public mu__int
{
 public:
  // constructors
  mu__byte(int lb, int ub, int size, const char *n, int os)
    :mu__int(lb, ub, size, n, os)
  {
  };
  mu__byte(int lb, int ub, int size)
    :mu__int(lb, ub, size)
  {
  };
  mu__byte(int lb, int ub, int size, int val)
    :mu__int(lb, ub, size, val)
  {
  };

  // assignment
  int operator=(int val)
  {
    return mu__int::operator=(val);
  }

};

/****************************************
  The class for a long int
 ****************************************/

// Uli: - this class redefines most almost all functions to access the
//        data
//      - this is necessary because of the different layout of the data
//      - templates may provide a more elegant solution
//        however: pointer-access to the values in the state vector is
//                 not possible since they are not individually aligned

class mu__long:public mu__int
{
  enum { undef_value = UNDEFVALLONG };

 public:
  int cvalue;			// Uli: hides the cvalue from the base class mu__int

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // constructors

  // Uli: example problem with the constructors:
  // - the undefined() in the mu__int constructor does not set the mu__long
  //    cvalue
  // - the programming is not too nice, improving, however, seems to require
  //    lots of changes in the code

  mu__long(int lb, int ub, int size, const char *n, int os)
    :mu__int(lb, ub, size, n, os)
  {
    undefined();
  };
  mu__long(int lb, int ub, int size)
    :mu__int(lb, ub, size)
  {
    undefined();
  };
  mu__long(int lb, int ub, int size, int val)
    :mu__int(lb, ub, size, val)
  {
    operator=(val);
  };
  mu__long(const mu__long & src)
    :mu__int(src)
  {
    value(int (src));
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // data access routines

  inline void clear()
  {
    value(lb);
    initialized = TRUE;
  };
  inline void undefine()
  {
    undefined();
    initialized = TRUE;
  };
  inline void reset()
  {
    undefined();
    initialized = FALSE;
  };

  // assignment
#ifndef NO_RUN_TIME_CHECKING
  int operator=(int val)
  {
    if ((val <= ub) && (val >= lb)) {
      value(val);
      initialized = TRUE;
    } else
      boundary_error(val);
    return val;
  }
#else
  int operator=(int val)
  {
    return value(val);
  }
#endif

  // conversion to int
#ifndef NO_RUN_TIME_CHECKING
  inline operator   int () const
  {
    if (isundefined())
      return undef_error();
    return value();
  };
#else
  inline operator   int () const
  {
    return value();
  };
#endif

  // new data access routines
#ifdef ALIGN
  inline const int value() const
  {
    if (in_world)
      return workingstate->getlong(byteOffset);
    else
      return cvalue;
  };
  inline int value(int val)
  {
    if (in_world)
      workingstate->setlong(byteOffset, val);
    else
      cvalue = val;
    return val;
  };
  inline void undefined()
  {
    if (!in_world)
      cvalue = undef_value;
    else
      workingstate->setlong(byteOffset, undef_value);
  };
  inline void defined(bool val)
  {
    if (!val)
      if (!in_world)
        cvalue = undef_value;
      else
        workingstate->setlong(byteOffset, undef_value);
  };
  bool defined() const
  {
    if (in_world)
      return (workingstate->getlong(byteOffset) != undef_value);
    else
      return cvalue != undef_value;
  };
  bool isundefined() const
  {
    if (in_world)
      return (workingstate->getlong(byteOffset) == undef_value);
    else
      return cvalue == undef_value;
  };
#else
  // Uli: - code from mu__int has to be duplicated here
  //      - otherwise the wrong cvalue/undef_value is accessed
  inline const int value() const
  {
    if (in_world)
      return workingstate->get(&where) + lb - 1;
    else
      return cvalue + lb - 1;
  };
  inline int value(int val)
  {
    if (in_world)
      workingstate->set(&where, val - lb + 1);
    else
      cvalue = val - lb + 1;
    return val;
  };
  inline void defined(bool val)
  {
    if (!val)
      if (in_world)
        workingstate->set(&where, undef_value);
      else
        cvalue = undef_value;
  };
  inline bool defined() const
  {
    if (in_world)
      return (workingstate->get(&where) != undef_value);
    else
      return cvalue != undef_value;
  };
  inline void undefined()
  {
    defined(FALSE);
  };
  inline bool isundefined() const
  {
    if (in_world)
      return (workingstate->get(&where) == undef_value);
    else
      return cvalue == undef_value;
  };
#endif

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // printing, etc.

  // printing routine, added by Uli
  virtual void print(FILE *target=stdout, const char *separator="\n")
  {
    if (defined())
      fprintf(target,"%s: %d%s",name,value(),separator);
    else
      fprintf(target,"%s: Undefined%s",name,separator);
  }
  friend ostream & operator<<(ostream & s, mu__long & val)
  {
    if (val.defined())
      s << (int) val;
    else
      s << "Undefined";
    return s;
  }
  void print_diff(state * prev, FILE *target, const char *separator);

  // transfer routines
  void to_state(state * thestate)
  {
    int val = value();		// Uli: copy value (which is probably undefined)
    in_world = TRUE;
    value(val);
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // comparing routines, for symmetry

  friend int CompareWeight(mu__long & a, mu__long & b)
  {
    if (!a.defined() && !b.defined())
      return 0;
    else if (!a.defined())
      return -1;
    else if (!b.defined())
      return 1;
    else if (a.value() == b.value())
      return 0;
    else if (a.value() > b.value())
      return 1;
    else
      return -1;
  };

  friend int Compare(mu__long & a, mu__long & b)
  {
    if (!a.defined() && !b.defined())
      return 0;
    else if (!a.defined())
      return -1;
    else if (!b.defined())
      return 1;
    else if (a.value() == b.value())
      return 0;
    else if (a.value() > b.value())
      return 1;
    else
      return -1;
  };
};


/****************************************
  The class for a boolean
 ****************************************/

class mu_0_boolean:public mu__int
/* In the Murphi compilation, we typecheck against mixing ints and enums,
 * so now, we can allow it to be mixed with integers without catastrophe. */
{
  // extra data from mu__int
  static const char *values[];

  // special stream operation --> use names false or true
  friend ostream & operator<<(ostream & s, mu_0_boolean & x)
  {
    if (x.defined())
      return s << mu_0_boolean::values[int (x)];
    else
      return s << "Undefined";
  } public:
#ifndef NO_RUN_TIME_CHECKING
  inline int operator=(int val)
  {
    initialized = TRUE;
    return value(val ? 1 : 0);
  };
  inline int operator=(const mu_0_boolean & val)
  {
    initialized = TRUE;
    return value(val ? 1 : 0);
  };
#else
  inline int operator=(int val)
  {
    return value(val);
  };
  inline int operator=(const mu_0_boolean & val)
  {
    return value(val);
  };
#endif

  mu_0_boolean(const char *name, int os)
    :    mu__int(0, 1, 2, name, os)
  {
  };
  mu_0_boolean(void)
    :mu__int(0, 1, 2)
  {
  };
  mu_0_boolean(int val)
    :mu__int(0, 1, 2, "Parameter or function result", 0)
  {
    operator=(val);
  };

  // special assignment for boolean
  const char *Name()
  {
    return values[value()];
  };

  // canonicalization function for symmetry
  virtual void Permute(PermSet & Perm, int i);
  virtual void SimpleCanonicalize(PermSet & Perm);
  virtual void Canonicalize(PermSet & Perm);
  virtual void SimpleLimit(PermSet & Perm);
  virtual void ArrayLimit(PermSet & Perm);
  virtual void Limit(PermSet & Perm);

  // special print for boolean --> use name False or True
  virtual void print(FILE *target=stdout, const char *separator="\n")
  {
    if (defined())
      fprintf(target,"%s: %s%s",name,values[value()],separator);
    else
      fprintf(target,"%s: Undefined%s",name,separator);
  };
  void print_statistic()
  {
  };
};

/****************************************
  world_class declarations
 ****************************************/

class world_class
{
 public:
  void print(FILE *target=stdout, const char *separator="\n");			/* print out the values of all variables. */

  void fire_processes();	// fire the processes if they are enabled		WP
  void fire_processes_plus();	// fire the processes (add effects only) if they are enabled		WP
  void fire_processes_minus();	// fire the processes (delete effects only) if they are enabled		WP
  double get_f_val();	// get the f(n) = g(n)+h(n) value for a state		WP
  void set_f_val();		// set the f(n) = g(n)+h(n) value to a state		WP
  double get_h_val();	// get h(n) value							WP
  void set_h_val();		// set h(n) value (FF-RPG)					WP
  void set_h_val(int hp);		// set h(n) value with params		WP
  double get_g_val();			// get h(n) value					WP
  void set_g_val(double g_val);			// set g(n) value			WP

  void pddlprint(FILE *target=stdout, const char *separator="\n");		/* print out the values of all variables. */
#ifdef HAS_CLOCK
  double get_clock_value(); //GDP
#endif
  void print_statistic();
  void print_diff(state * prevstate, FILE *target=stdout, const char *separator="\n");
  void clear();			/* clear every variable. */
  void undefine();		/* undefine every variable. */
  void reset();			/* uninitialize every variable. */

  std::vector<mu_0_boolean*> get_mu_bools();		/* get single mu_0_boolean variables from state WP */
  std::vector<mu_0_boolean*> get_mu_bool_arrays();		/* get mu_0_boolean variables from arrays from state WP */
  std::vector<mu__real*> get_mu_nums();		/* get single mu_1_real_type variables from state WP */
  std::vector<mu__real*> get_mu_num_arrays();		/* get mu_1_real_type variables from arrays from state WP */


  /*
   * this is something of a different approach than previous versions--
   * we let the translation of the world
   * into states be a property of the world,
   * instead of a property of the state.
   */
  void to_state(state * newstate);	/* encode the world into newstate. */
  state *getstate();		/* return a state encoding the values of all variables. */
  state_L *getstate_L();	/* return a state_L */
  void setstate(state * thestate);	/*set the state of the world to
					   /*thestate. */
};

/***************************
  Timer
 ***************************/
double SecondsSinceStart(void);


/***************************   // added by Uli
  random number generator
 ***************************/
class randomGen
// random number generator
{
 private:
  unsigned long value;
 public:
  randomGen();
  unsigned long next();		// return next random number
};
