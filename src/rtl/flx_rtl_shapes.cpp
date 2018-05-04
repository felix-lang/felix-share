#line 1171 "/Users/skaller/felix/src/packages/rtl.fdoc"
#include "flx_rtl_shapes.hpp"
#include "flx_rtl.hpp"
//#include "flx_collector.hpp"
#include "flx_dynlink.hpp"
#include <stddef.h>

namespace flx { namespace rtl {


// ********************************************************
//OFFSETS for slist_node_t
// ********************************************************
static const std::size_t slist_node_offsets[2]={
    offsetof(slist_node_t,next),
    offsetof(slist_node_t,data)
};

static ::flx::gc::generic::offset_data_t const slist_node_offset_data = { 2, slist_node_offsets };
::flx::gc::generic::gc_shape_t slist_node_ptr_map = {
  NULL,
  "rtl::slist_node_t",
  1,sizeof(slist_node_t),
  0, // no finaliser,
  0, // fcops
  &slist_node_offset_data,
  ::flx::gc::generic::scan_by_offsets,
  ::flx::gc::generic::tblit<slist_node_t>,::flx::gc::generic::tunblit<slist_node_t>,
  ::flx::gc::generic::gc_flags_default,
  0UL, 0UL
};


// ********************************************************
//OFFSETS for slist_t
// ********************************************************
static const std::size_t slist_offsets[1]={
    offsetof(slist_t,head)
};
static ::flx::gc::generic::offset_data_t const slist_offset_data = { 1, slist_offsets };

static CxxValueType<slist_t> _slist_t_fcops {};

::flx::gc::generic::gc_shape_t slist_ptr_map = {
  &slist_node_ptr_map,
  "rtl::slist_t",
  1,sizeof(slist_t),
  0, // no finaliser
  &_slist_t_fcops, // fcops
  &slist_offset_data,
  ::flx::gc::generic::scan_by_offsets,
  ::flx::gc::generic::tblit<slist_t>,::flx::gc::generic::tunblit<slist_t>,
  ::flx::gc::generic::gc_flags_default,
  0UL, 0UL
};


// ********************************************************
//OFFSETS for fthread_t
// ********************************************************
static const std::size_t _fthread_offsets[1]={
    offsetof(fthread_t,cc)
};

static ::flx::gc::generic::offset_data_t const _fthread_offset_data = { 1, _fthread_offsets };

::flx::gc::generic::gc_shape_t _fthread_ptr_map = {
  &slist_ptr_map,
  "rtl::fthread_t",
  1,sizeof(fthread_t),
  0,
  0, // fcops
  &_fthread_offset_data,
  ::flx::gc::generic::scan_by_offsets,
  ::flx::gc::generic::tblit<fthread_t>,::flx::gc::generic::tunblit<fthread_t>,
  gc::generic::gc_flags_immobile,
  0UL, 0UL
};


// ********************************************************
//OFFSETS for schannel_t
// ********************************************************
static const std::size_t schannel_offsets[2]={
    offsetof(schannel_t,waiting_to_read),
    offsetof(schannel_t,waiting_to_write)
};

static ::flx::gc::generic::offset_data_t const schannel_offset_data = { 2, schannel_offsets };

::flx::gc::generic::gc_shape_t schannel_ptr_map = {
  &_fthread_ptr_map,
  "rtl::schannel_t",
  1,sizeof(schannel_t),
  0, // no finaliser
  0, // fcops
  &schannel_offset_data, // scanner data
  ::flx::gc::generic::scan_by_offsets, // scanner
  ::flx::gc::generic::tblit<schannel_t>,  // encoder
  ::flx::gc::generic::tunblit<schannel_t>,  // decoder
  gc::generic::gc_flags_default,
  0UL, 0UL
};

// ********************************************************
// _uctor_ implementation
// ********************************************************
//OFFSETS for _uctor_
static const std::size_t _uctor_offsets[1]= {
  offsetof(_uctor_,data)
};

static ::flx::gc::generic::offset_data_t const _uctor_offset_data = { 1, _uctor_offsets };

static CxxValueType<_uctor_> _uctor_fcops {};

::flx::gc::generic::gc_shape_t _uctor_ptr_map = {
  &schannel_ptr_map,
  "rtl::_uctor_",
  1,
  sizeof(_uctor_),
  0, // finaliser
  &_uctor_fcops, // fcops
  &_uctor_offset_data, // scanner data
  ::flx::gc::generic::scan_by_offsets, // scanner
  ::flx::gc::generic::tblit<_uctor_>, // encoder
  ::flx::gc::generic::tunblit<_uctor_>,  // decoder
  gc::generic::gc_flags_default
};

/*
// ********************************************************
// _variant_ implementation
// ********************************************************
//OFFSETS for _variant_
static const std::size_t _variant_offsets[1]= {
  offsetof(_variant_,vdata)
};

static CxxValueType<_variant_> _variant_fcops {};

static ::flx::gc::generic::offset_data_t const _variant_offset_data = { 1, _variant_offsets };

::flx::gc::generic::gc_shape_t _variant_ptr_map = {
  &_uctor_ptr_map,
  "rtl::_variant_",
  1,
  sizeof(_variant_),
  0, // finaliser
  &_variant_fcops, // fcops
  &_variant_offset_data, // scanner data
  ::flx::gc::generic::scan_by_offsets, // scanner
  ::flx::gc::generic::tblit<_variant_>, // encoder
  ::flx::gc::generic::tunblit<_variant_>,  // decoder
  gc::generic::gc_flags_default
};
*/

static CxxValueType<int> int_fcops {};

// ********************************************************
// jump_address implementation
// ********************************************************
//OFFSETS for jump_address
static const std::size_t jump_address_offsets[1]= {
  offsetof(jump_address_t,target_frame)
};

static ::flx::gc::generic::offset_data_t const
  jump_address_offset_data = { 1, jump_address_offsets }
;

static CxxValueType<jump_address_t> jump_address_t_fcops {};

::flx::gc::generic::gc_shape_t jump_address_ptr_map = {
  &_uctor_ptr_map,
  "rtl::jump_address_t",
  1,
  sizeof(_uctor_),
  0, // finaliser
  &jump_address_t_fcops, // fcops
  &jump_address_offset_data, // scanner data
  ::flx::gc::generic::scan_by_offsets, // scanner
  ::flx::gc::generic::tblit<jump_address_t>, // encoder
  ::flx::gc::generic::tunblit<jump_address_t>,  // decoder
  gc::generic::gc_flags_default
};

// ********************************************************
// int implementation
// ********************************************************


::flx::gc::generic::gc_shape_t _int_ptr_map = {
  &jump_address_ptr_map,
  "rtl::int",
  1,
  sizeof(int),
  0, // finaliser
  &int_fcops,
  //0, // fcops
  0, // scanner data
  0, // scanner
  ::flx::gc::generic::tblit<int>, // encoder
  ::flx::gc::generic::tunblit<int>,  // decoder
  gc::generic::gc_flags_default,
  0UL, 0UL
};

// ********************************************************
// cl_t implementation
// ********************************************************

static CxxValueType<cl_t> cl_t_fcops {};

::flx::gc::generic::gc_shape_t cl_t_ptr_map = {
  &_int_ptr_map,
  "rtl::cl_t",
  1,
  sizeof(cl_t),
  0, // finaliser
  &cl_t_fcops, // fcops
  0, // scanner data
  0, // scanner
  ::flx::gc::generic::tblit<cl_t>,
  ::flx::gc::generic::tunblit<cl_t>,
  gc::generic::gc_flags_default,
  0UL, 0UL
};

// ********************************************************
// clptr_t implementation
// ********************************************************

static CxxValueType<clptr_t> clptr_t_fcops {};

static const std::size_t _clptr_t_offsets[1]={ 0 };
::flx::gc::generic::offset_data_t const _clptr_t_offset_data = { 1, _clptr_t_offsets };


::flx::gc::generic::gc_shape_t clptr_t_ptr_map = {
  &cl_t_ptr_map,
  "rtl::clptr_t",
  1,
  sizeof(clptr_t),
  0, // finaliser
  &clptr_t_fcops, // fcops
  &_clptr_t_offset_data, // scanner data
  ::flx::gc::generic::scan_by_offsets, // scanner
  ::flx::gc::generic::tblit<clptr_t>,
  ::flx::gc::generic::tunblit<clptr_t>,
  gc::generic::gc_flags_default,
  0UL, 0UL
};

// ********************************************************
// clprj_t implementation
// ********************************************************

static CxxValueType<clprj_t> clprj_t_fcops {};

::flx::gc::generic::offset_data_t const _clprj_t_offset_data = { 0, NULL };


::flx::gc::generic::gc_shape_t clprj_t_ptr_map = {
  &clptr_t_ptr_map,
  "rtl::clprj_t",
  1,
  sizeof(clprj_t),
  0, // finaliser
  &clprj_t_fcops, // fcops
  0, // scanner data
  ::flx::gc::generic::scan_by_offsets, // scanner
  ::flx::gc::generic::tblit<clprj_t>,
  ::flx::gc::generic::tunblit<clprj_t>,
  gc::generic::gc_flags_default,
  0UL, 0UL
};


// ********************************************************
// pointer implementation
// ********************************************************

//OFFSETS for address
static const std::size_t _address_offsets[1]={ 0 };
::flx::gc::generic::offset_data_t const _address_offset_data = { 1, _address_offsets };

static ::std::string address_encoder (void *p) {
  return ::flx::gc::generic::blit (p,sizeof (void*));
}

static size_t address_decoder (void *p, char *s, size_t i) {
  return ::flx::gc::generic::unblit (p,sizeof (void*),s,i);
}


// ********************************************************
// address implementation : MUST BE LAST because the compiler
// uses "address_ptr_map" as the back link for generated shape tables
// ********************************************************

::flx::gc::generic::gc_shape_t _address_ptr_map = {
  &clprj_t_ptr_map,
  "rtl::address",
  1,
  sizeof(void*),
  0, // finaliser
  0, // fcops
  &_address_offset_data, /// scanner data
  ::flx::gc::generic::scan_by_offsets, // scanner
  ::flx::gc::generic::tblit<void*>, // encoder
  ::flx::gc::generic::tunblit<void*>, // decoder
  gc::generic::gc_flags_default,
  0UL, 0UL
};


}}

