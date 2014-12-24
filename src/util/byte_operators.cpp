/*******************************************************************\

Module:

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include <ostream>
#include <cassert>

#include "std_types.h"
#include "pointer_offset_size.h"
#include "arith_tools.h"
#include "byte_operators.h"
#include "namespace.h"
#include "config.h"

/*******************************************************************\

Function: byte_extract_id

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

irep_idt byte_extract_id()
{
  switch(config.ansi_c.endianness)
  {
  case configt::ansi_ct::IS_LITTLE_ENDIAN:
    return ID_byte_extract_little_endian;

  case configt::ansi_ct::IS_BIG_ENDIAN:
    return ID_byte_extract_big_endian;

  default:
    assert(false);
  }
}

/*******************************************************************\

Function: byte_update_id

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

irep_idt byte_update_id()
{
  switch(config.ansi_c.endianness)
  {
  case configt::ansi_ct::IS_LITTLE_ENDIAN:
    return ID_byte_update_little_endian;

  case configt::ansi_ct::IS_BIG_ENDIAN:
    return ID_byte_update_big_endian;

  default:
    assert(false);
  }
}

/*******************************************************************\

Function: endianness_mapt::output

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void endianness_mapt::output(std::ostream &out) const
{
  for(std::vector<size_t>::const_iterator it=map.begin();
      it!=map.end();
      ++it)
  {
    if(it!=map.begin()) out << ", ";
    out << *it;
  }
}

/*******************************************************************\

Function: endianness_mapt::build_little_endian

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void endianness_mapt::build_little_endian(const typet &src)
{
  mp_integer s=pointer_offset_size(ns, src); // error is -1

  while(s>0)
  {
    map.push_back(map.size());
    --s;
  }

}

/*******************************************************************\

Function: endianness_mapt::build_big_endian

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void endianness_mapt::build_big_endian(const typet &src)
{
  if(src.id()==ID_symbol)
    build_big_endian(ns.follow(src));
  else if(src.id()==ID_c_enum_tag)
    build_big_endian(ns.follow_tag(to_c_enum_tag_type(src)));
  else if(src.id()==ID_unsignedbv ||
          src.id()==ID_signedbv ||
          src.id()==ID_fixedbv ||
          src.id()==ID_floatbv ||
          src.id()==ID_c_enum)
  {
    mp_integer s=pointer_offset_size(ns, src); // error is -1
    assert(s>=0);

    size_t s_int=integer2long(s), base=map.size();

    for(size_t i=0; i<s_int; i++)
      map.push_back(base+s_int-i-1); // these do get re-ordered!
  }
  else if(src.id()==ID_struct)
  {
    const struct_typet &struct_type=to_struct_type(src);
    
    size_t bit_field_bits=0;

    // todo: worry about padding being in wrong order
    for(struct_typet::componentst::const_iterator
        it=struct_type.components().begin();
        it!=struct_type.components().end();
        it++)
    {
      if(it->type().id()==ID_c_bit_field)
      {
        unsigned bits=to_c_bit_field_type(it->type()).get_width();
        unsigned bytes=0;

        assert(bit_field_bits<8);

        if(bit_field_bits+bits<8)
        {
          // keep accumulating
          bit_field_bits+=bits;
        }
        else
        {
          bit_field_bits+=bits;
          bytes=bit_field_bits/8;
          bit_field_bits-=bytes*8;

          size_t base=map.size();

          for(size_t i=0; i<bytes; i++)
            map.push_back(base+bytes-i-1); // these do get re-ordered!
        }
      }
      else
        build_big_endian(it->type());
    }
  }
  else if(src.id()==ID_array)
  {
    const array_typet &array_type=to_array_type(src);

    // array size constant?
    mp_integer s;
    if(!to_integer(array_type.size(), s))
    {
      while(s>0)
      {
        build_big_endian(array_type.subtype());
        --s;
      }
    }
  }
  else
  {
    // everything else (unions in particular)
    // is treated like a byte-array
    mp_integer s=pointer_offset_size(ns, src); // error is -1
    while(s>0)
    {
      map.push_back(map.size());
      --s;
    }
  }
}
