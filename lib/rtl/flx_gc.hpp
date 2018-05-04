#line 174 "/Users/skaller/felix/src/packages/gc.fdoc"

#ifndef __FLX_GC_H__
#define __FLX_GC_H__

#include <cstdlib>
#include <stddef.h>
#include "flx_gc_config.hpp"
#include "pthread_thread_control_base.hpp"
#include <string>
#include "flx_compiler_support_bodies.hpp"
#include <chrono>

// we use an STL set to hold the collection of roots
#include <set>

namespace flx {
namespace gc {
namespace generic {
// Here are the types we refer to:

struct GC_EXTERN gc_shape_t;      // the shape of collectable objects
struct GC_EXTERN collector_t;     // the collector itself
struct GC_EXTERN allocator_t;     // the allocator used
struct GC_EXTERN offset_data_t;   // private data for offset scanner
struct GC_EXTERN pointer_data_t;  // description of a pointer

#line 215 "/Users/skaller/felix/src/packages/gc.fdoc"
struct GC_EXTERN pointer_data_t
{
  void *pointer;                      //< candidate pointer
  void *head;                         //< head object
  size_t max_elements;         //< allocated slots
  size_t used_elements;        //< used slots
  gc_shape_t *shape;                  //< shape
};

#line 228 "/Users/skaller/felix/src/packages/gc.fdoc"
enum gc_shape_flags_t {
  gc_flags_default    = 0,            //< collectable and mobile
  gc_flags_immobile   = 1,            //< cannot be moved
  gc_flags_persistent = 2,            //< cannot be deallocated
  gc_flags_conservative = 4           //< scan whole object conservatively
};

/// Describes runtime object shape.
typedef void finaliser_t (collector_t*, void*);
typedef void *scanner_t(collector_t*, gc_shape_t *, void *, size_t, int);
typedef ::std::string encoder_t (void *);
typedef ::std::size_t decoder_t(void *, char *, ::std::size_t);
typedef void copier_t (void*,void*);
typedef void dflt_init_t (void*);

struct GC_EXTERN gc_shape_t
{
  gc_shape_t *next_shape;         ///< pointer to next shape in list or NULL
  char const *cname;              ///< C++ typename
  ::std::size_t count;            ///< static array element count
  ::std::size_t amt;              ///< bytes allocated
  finaliser_t *finaliser;         ///< finalisation function
  ValueType *fcops;               ///< first class ops
/*
  copier_t *copy_init;
  copier_t *move_init;
  copier_t *copy_assign;
  copier_t *move_assign;
*/
  void const *private_data;       ///< private data passed to scanner
  scanner_t *scanner;             ///< scanner function
  encoder_t *encoder;             ///< encoder function
  decoder_t *decoder;             ///< encoder function
  gc_shape_flags_t flags;         ///< flags
  size_t allocations;
  size_t deallocations;
};

GC_EXTERN extern gc_shape_t _ptr_void_map;

#line 273 "/Users/skaller/felix/src/packages/gc.fdoc"
struct GC_EXTERN offset_data_t
{
  ::std::size_t n_offsets;
  ::std::size_t const *offsets;
};

GC_EXTERN scanner_t scan_by_offsets;

#line 290 "/Users/skaller/felix/src/packages/gc.fdoc"

/*
 * The following template is provided as a standard wrapper
 * for C++ class destructors. The term std_finaliser<T>
 * denotes a function pointer to the wrapper for the destructor
 * of class T, which can be used as a finaliser in the shape
 * descriptor of a T. The client is cautioned than the order
 * of finalisation may not be what is expected. Finalisers
 * should be provided for all C++ objects managed by the Felix
 * collector and not refering to Felix objects,
 * but which contain pointers to other objects that need
 * to be deleted when the main object is destroyed;
 * for example a string class managing an array of char
 * requires its destructor be invoked to delete the managed
 * array, and so a finaliser wrapping the destructor must
 * be provided.
 *
 * C data types may, of course, also require destruction,
 * and Felix therefore can provide programmers with
 * the convenience of C++ destructors, even for C data types.
 */
template<class T>
void std_finaliser(collector_t*, void *t)
{
  static_cast<T*>(t) -> ~T();
}

#line 326 "/Users/skaller/felix/src/packages/gc.fdoc"
/// Allocator abstraction.

struct allocator_t {
  bool debug;
  allocator_t():debug(false){}
  virtual void *allocate(::std::size_t)=0;
  virtual void deallocate(void *)=0;
  virtual ~allocator_t();
  void set_debug(bool d){debug=d;}
};

#line 357 "/Users/skaller/felix/src/packages/gc.fdoc"

/// Collector abstraction.
struct GC_EXTERN collector_t
{
  bool debug;
  bool report_gcstats;
  void *module_registry;
  void set_debug(bool d, bool stats){debug=d;report_gcstats=stats;}
  collector_t();
  virtual ~collector_t();
  virtual ::flx::pthread::thread_control_base_t *get_thread_control()const =0;
  virtual void register_pointer(void *q, int reclimit)=0;
  ::std::chrono::time_point<::std::chrono::high_resolution_clock> start_time;
  ::std::chrono::duration<double> gc_time;

  virtual bool inrange(void *)const =0;
  // These routines just provide statistics.
  size_t get_allocation_count()const {
    return v_get_allocation_count();
  }

  size_t get_root_count()const {
    return v_get_root_count();
  }

  size_t get_allocation_amt()const {
    return v_get_allocation_amt();
  }

  // Hooks for the supplied allocator, which operate in
  // terms of shape objects rather than raw memory amounts.
  void *allocate(gc_shape_t *shape, size_t x) {
    return v_allocate(shape,x);
  }

  // The mark and sweep collector algorithm.
  size_t collect() {
    //fprintf(stderr, "Collecting\n");
    ::std::chrono::time_point< ::std::chrono::high_resolution_clock> start_time, end_time;
    start_time = ::std::chrono::high_resolution_clock::now();
    size_t x = v_collect();
    end_time = ::std::chrono::high_resolution_clock::now();
    ::std::chrono::duration<double> elapsed = end_time - start_time;

    if (debug)
      fprintf(stderr, "Collecting DONE in %10.5f seconds\n", elapsed.count());
    gc_time += elapsed;
    return x;
  }

  // Routines to add and remove roots.
  void add_root(void *memory) {
    v_add_root(memory);
  }

  void remove_root(void *memory) {
    v_remove_root(memory);
  }

  void free_all_mem() {
    //fprintf(stderr,"Dispatching to free all mem\n");
    v_free_all_mem();
  }

  void finalise(void *frame) {
    v_finalise(frame);
  }

  // Integrity check for the data structure being managed.
  // array management
  virtual void set_used(void *memory, size_t)=0;
  virtual void incr_used(void *memory, ptrdiff_t)=0;
  virtual size_t get_used(void *memory)=0;
  virtual size_t get_count(void *memory)=0;
  virtual void *create_empty_array( gc_shape_t *shape, size_t count)=0;

  virtual pointer_data_t get_pointer_data(void *)=0;
private:
  virtual size_t v_get_allocation_count()const=0;
  virtual size_t v_get_root_count()const=0;
  virtual size_t v_get_allocation_amt()const=0;
  virtual void *v_allocate(gc_shape_t *shape, size_t)=0;
  virtual void v_finalise(void *fp)=0;
  virtual size_t v_collect()=0;
  virtual void v_add_root(void *memory)=0;
  virtual void v_remove_root(void *memory)=0;
  virtual void v_free_all_mem()=0;

  // It doesn't make any sense to copy collector objects
  // about.
  void operator=(collector_t const&);
  collector_t(collector_t const&);
};

// The gc_profile_t is a grab bag of controls related to the collector.
struct GC_EXTERN gc_profile_t {
  bool debug_driver;
  bool debug_allocations;     ///< allocator debug on/off
  bool debug_collections;     ///< collector debug on/off
  bool report_collections;    ///< collector debug on/off
  bool report_gcstats;        ///< print final gc statistics
  bool allow_collection_anywhere; ///< enable collect on allocate

  size_t gc_freq;      ///< how often to collect
  size_t gc_counter;   ///< counter to check if time to collect

  size_t min_mem;      ///< min memory before collection
  size_t max_mem;      ///< throw out of memory if above here
  size_t threshhold;   ///< collection trigger point
  double free_factor;         ///< reset threshhold to used memory
                              ///< by this factor after collection

  size_t collections;  ///< number of collections done
  bool finalise;              ///< whether Felix should collect on exit
  flx::gc::generic::collector_t *collector;

  size_t maybe_collect(); ///< function which maybe collects
  size_t actually_collect(); ///< function which actually collects

  void *allocate(
    flx::gc::generic::gc_shape_t *shape,
    size_t count,
    bool allow_gc
  );

  gc_profile_t (
    bool debug_driver_,
    bool debug_allocations_,
    bool debug_collections_,
    bool report_collections_,
    bool report_gcstats_,
    bool allow_collection_anywhere_,
    size_t gc_freq_,
    size_t min_mem_,
    size_t max_mem_,
    double free_factor_,
    bool finalise_,
    flx::gc::generic::collector_t *collector
  );
  ~gc_profile_t();
};

}}} // end namespaces

/*
 * The following two routines are used to provide
 * C++ type safe heap allocation. There are no corresponding
 * delete routines, please use the destroy function.
 *
 * Note these routines are now placed
 * in the global namespace to accomodate Metrowerks
 * compiler on Mac OS.
 */
GC_EXTERN void *operator new
(
  ::std::size_t,
  flx::gc::generic::gc_profile_t &,
  flx::gc::generic::gc_shape_t &,
  bool
);

/*
 * Define an empty delete to make msvc happy.
 */
GC_EXTERN void operator delete(
  void*,
  flx::gc::generic::gc_profile_t &,
  flx::gc::generic::gc_shape_t &,
  bool
);

#endif
