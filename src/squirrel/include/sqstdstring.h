/*	see copyright notice in squirrel.h */
#ifndef _SQSTD_STRING_H_
#define _SQSTD_STRING_H_

typedef unsigned int SQRexBool;
typedef struct SQRex SQRex;

typedef struct {
	const SQChar *begin;
	SQInteger len;
} SQRexMatch;

SQUIRREL_API SQRex *sqstd_rex_compile(const SQChar *pattern,const SQChar **error);
SQUIRREL_API void sqstd_rex_free(SQRex *exp);
SQUIRREL_API SQBool sqstd_rex_match(SQRex* exp,const SQChar* text);
SQUIRREL_API SQBool sqstd_rex_search(SQRex* exp,const SQChar* text, const SQChar** out_begin, const SQChar** out_end);
SQUIRREL_API SQBool sqstd_rex_searchrange(SQRex* exp,const SQChar* text_begin,const SQChar* text_end,const SQChar** out_begin, const SQChar** out_end);
SQUIRREL_API SQInteger sqstd_rex_getsubexpcount(SQRex* exp);
SQUIRREL_API SQBool sqstd_rex_getsubexp(SQRex* exp, SQInteger n, SQRexMatch *subexp);

template <Squirk Q>
SQUIRREL_API SQRESULT sqstd_format(HSQUIRRELVM<Q> v,SQInteger nformatstringidx,SQInteger *outlen,SQChar **output);

template <Squirk Q>
SQUIRREL_API SQRESULT sqstd_register_stringlib(HSQUIRRELVM<Q> v);

#endif /*_SQSTD_STRING_H_*/
