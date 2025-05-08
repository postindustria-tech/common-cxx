/* *********************************************************************
 * This Original Work is copyright of 51 Degrees Mobile Experts Limited.
 * Copyright 2023 51 Degrees Mobile Experts Limited, Davidson House,
 * Forbury Square, Reading, Berkshire, United Kingdom RG1 3EU.
 *
 * This Original Work is licensed under the European Union Public Licence
 * (EUPL) v.1.2 and is subject to its terms as set out below.
 *
 * If a copy of the EUPL was not distributed with this file, You can obtain
 * one at https://opensource.org/licenses/EUPL-1.2.
 *
 * The 'Compatible Licences' set out in the Appendix to the EUPL (as may be
 * amended by the European Commission) shall be deemed incompatible for
 * the purposes of the Work and the provisions of the compatibility
 * clause in Article 5 of the EUPL shall not apply.
 *
 * If using the Work as, or as part of, a network application, by
 * including the attribution notice(s) required under Article 5 of the EUPL
 * in the end user terms of the application under an appropriate heading,
 * such notice(s) shall fulfill the requirements of that article.
 * ********************************************************************* */


/* Byte Array Mapping */
%include "arrays_java.i"

%typemap(ctype) (unsigned char *UCHAR) "unsigned char*"
%typemap(jtype) (unsigned char *UCHAR) "byte[]"
%typemap(jstype) (unsigned char *UCHAR) "byte[]"
%typemap(jni) (unsigned char *UCHAR) "jbyteArray"

%apply unsigned char *UCHAR {unsigned char data[]}
%apply unsigned char *UCHAR {unsigned char ipAddress[]}
%apply unsigned char *UCHAR {unsigned char copy[]}

%include std_vector.i
%include std_string.i
%include various.i


%typemap(jni) (unsigned char *UCHAR, long LENGTH) "jbyteArray"
%typemap(jtype) (unsigned char *UCHAR, long LENGTH) "byte[]"
%typemap(jstype) (unsigned char *UCHAR, long LENGTH) "byte[]"
%typemap(in) (unsigned char *UCHAR, long LENGTH) {
    $1 = JCALL2(GetByteArrayElements, jenv, $input, NULL);
    $2 = JCALL1(GetArrayLength, jenv, $input);
}


%typemap(javain) (unsigned char *UCHAR, long LENGTH) "$javainput"

/* Prevent default freearg typemap from being used */
%typemap(freearg) (unsigned char *UCHAR, long LENGTH) ""

%apply (unsigned char *UCHAR, long LENGTH) { (unsigned char data[], long length) };

/* Use byte correctly where methods would otherwise not take a proper type. */
%typemap(jni) (unsigned char UCHAR) "int"
%typemap(jtype) (unsigned char UCHAR) "int"
%typemap(jstype) (unsigned char UCHAR) "byte"
%typemap(javain) (unsigned char UCHAR) "(int)$javainput"
%typemap(in) (unsigned char UCHAR) {
    $1 = (byte)$input;
  }
%typemap(out) (unsigned char UCHAR) "$result = (int)$1;"
%typemap(javaout) (unsigned char UCHAR) {
    return (byte)$jnicall;
  }
/* Prevent default freearg typemap from being used */
%typemap(freearg) (unsigned char UCHAR) ""
%apply (unsigned char UCHAR) { (byte) };

%define autocloseable(name)
%typemap(javainterfaces) name "AutoCloseable";
%typemap(javacode) name %{
  @Override
  public void close() {
    this.delete();
  }
%}
%enddef

%define nofinalize(name)
%typemap(javafinalize) name %{%}
%enddef


autocloseable(EvidenceBase);
autocloseable(Value);
autocloseable(std::map);
autocloseable(std::vector);
%extend std::vector {
    %typemap(javainterfaces) std::vector "AutoCloseable, java.util.RandomAccess";
};
autocloseable(ResultsBase);
autocloseable(EngineBase);

nofinalize(ResultsBase)
nofinalize(EvidenceBase);
nofinalize(Value);
nofinalize(std::map);
nofinalize(std::vector);
