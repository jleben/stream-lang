#pragma once

#include "types.hpp"

namespace arrp {

class built_in_types
{
public:
    built_in_types();

    type_cons_ptr array(type_ptr elem);
    type_cons_ptr function(type_ptr param, type_ptr result);

    type_cons_ptr integer32() { return m_integer32; }
    type_cons_ptr integer64() { return m_integer64; }
    type_cons_ptr real32() { return m_real32; }
    type_cons_ptr real64() { return m_real64; }
    type_cons_ptr complex32() { return m_complex32; }
    type_cons_ptr complex64() { return m_complex64; }

    type_class_ptr integral() { return m_integral; }
    type_class_ptr real() { return m_real; }
    type_class_ptr complex() { return m_complex; }
    type_class_ptr numeric() { return m_numeric; }
    type_class_ptr indexable() { return m_indexable; }

private:
    type_constructor_ptr m_array_cons;
    type_constructor_ptr m_func_cons;

    type_cons_ptr m_integer32;
    type_cons_ptr m_integer64;
    type_cons_ptr m_real32;
    type_cons_ptr m_real64;
    type_cons_ptr m_complex32;
    type_cons_ptr m_complex64;

    type_class_ptr m_integral;
    type_class_ptr m_real;
    type_class_ptr m_complex;
    type_class_ptr m_numeric;
    type_class_ptr m_indexable;
};

}
