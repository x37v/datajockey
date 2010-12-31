#include <ruby.h>

//#include <QtDebug>
#include <qtruby.h>

#include "smoke/datajockey_audio_smoke.h"

#include <iostream>

static VALUE getClassList(VALUE /*self*/)
{
    VALUE classList = rb_ary_new();
    for (int i = 1; i <= datajockey_audio_Smoke->numClasses; i++) {
        if (datajockey_audio_Smoke->classes[i].className && !datajockey_audio_Smoke->classes[i].external)
            rb_ary_push(classList, rb_str_new2(datajockey_audio_Smoke->classes[i].className));
    }
    return classList;
}

const char*
resolve_classname_datajockey_audio(smokeruby_object * o)
{
    return qtruby_modules[o->smoke].binding->className(o->classId);
}

//extern TypeHandler Phonon_handlers[];

extern "C" {

VALUE datajockey_module;
VALUE datajockey_audio_module;
VALUE datajockey_audio_internal_module;

static QtRuby::Binding binding;

Q_DECL_EXPORT void
Init_datajockey_audio()
{
    init_datajockey_audio_Smoke();

    binding = QtRuby::Binding(datajockey_audio_Smoke);

    smokeList << datajockey_audio_Smoke;

    QtRubyModule module = { "DataJockey::Audio", resolve_classname_datajockey_audio, 0, &binding };
    qtruby_modules[datajockey_audio_Smoke] = module;

    //install_handlers(Phonon_handlers);

    datajockey_module = rb_define_module("DataJockey");
    datajockey_audio_module = rb_define_module_under(datajockey_module, "Audio");
    datajockey_audio_internal_module = rb_define_module_under(datajockey_audio_module, "Internal");

    rb_define_singleton_method(datajockey_audio_internal_module, "getClassList", (VALUE (*) (...)) getClassList, 0);

    rb_require("datajockey/datajockey_audio_internal.rb");
    rb_funcall(datajockey_audio_internal_module, rb_intern("init_all_classes"), 0);
}

}
