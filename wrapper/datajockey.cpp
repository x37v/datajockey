#include <ruby.h>

//#include <QtDebug>
#include <qtruby.h>

#include "smoke/datajockey_smoke.h"

#include <iostream>

static VALUE getClassList(VALUE /*self*/)
{
    VALUE classList = rb_ary_new();
    for (int i = 1; i <= datajockey_Smoke->numClasses; i++) {
        if (datajockey_Smoke->classes[i].className && !datajockey_Smoke->classes[i].external)
            rb_ary_push(classList, rb_str_new2(datajockey_Smoke->classes[i].className));
    }
    return classList;
}

const char*
resolve_classname_datajockey(smokeruby_object * o)
{
    return qtruby_modules[o->smoke].binding->className(o->classId);
}

//extern TypeHandler Phonon_handlers[];

extern "C" {

VALUE datajockey_module;
VALUE datajockey_internal_module;

static QtRuby::Binding binding;

Q_DECL_EXPORT void
Init_datajockey()
{
    init_datajockey_Smoke();

    binding = QtRuby::Binding(datajockey_Smoke);

    smokeList << datajockey_Smoke;

    QtRubyModule module = { "DataJockey", resolve_classname_datajockey, 0, &binding };
    qtruby_modules[datajockey_Smoke] = module;

    //install_handlers(Phonon_handlers);

    datajockey_module = rb_define_module("DataJockey");
    datajockey_internal_module = rb_define_module_under(datajockey_module, "Internal");

    rb_define_singleton_method(datajockey_internal_module, "getClassList", (VALUE (*) (...)) getClassList, 0);

    rb_require("datajockey/datajockey.rb");
    rb_funcall(datajockey_internal_module, rb_intern("init_all_classes"), 0);
}

}
