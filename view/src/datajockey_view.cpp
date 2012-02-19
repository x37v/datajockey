#include <ruby.h>

#include <QtDebug>
#include <qtruby.h>

#include "smoke/datajockey_view_smoke.h"

#include <iostream>

static VALUE getClassList(VALUE /*self*/)
{
    VALUE classList = rb_ary_new();
    for (int i = 1; i <= datajockey_view_Smoke->numClasses; i++) {
        if (datajockey_view_Smoke->classes[i].className && !datajockey_view_Smoke->classes[i].external)
            rb_ary_push(classList, rb_str_new2(datajockey_view_Smoke->classes[i].className));
    }
    return classList;
}

const char*
resolve_classname_datajockey_view(smokeruby_object * o)
{
    return qtruby_modules[o->smoke].binding->className(o->classId);
}

extern "C" {

VALUE datajockey_module;
VALUE datajockey_view_module;
VALUE datajockey_view_internal_module;

static QtRuby::Binding binding;

Q_DECL_EXPORT void
Init_datajockey_view()
{
    init_datajockey_view_Smoke();

    binding = QtRuby::Binding(datajockey_view_Smoke);

    smokeList << datajockey_view_Smoke;

    QtRubyModule module = { "DataJockey::View", resolve_classname_datajockey_view, 0, &binding };
    qtruby_modules[datajockey_view_Smoke] = module;

    datajockey_module = rb_define_module("DataJockey");
    datajockey_view_module = rb_define_module_under(datajockey_module, "View");
    datajockey_view_internal_module = rb_define_module_under(datajockey_view_module, "Internal");

    rb_define_singleton_method(datajockey_view_internal_module, "getClassList", (VALUE (*) (...)) getClassList, 0);

    rb_require("datajockey/datajockey_view_internal.rb");
    rb_funcall(datajockey_view_internal_module, rb_intern("init_all_classes"), 0);
}

}
