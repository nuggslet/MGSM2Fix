/*	see copyright notice in squirrel.h */
#ifndef _SQSTRING_H_
#define _SQSTRING_H_

inline SQHash _hashstr (const SQChar *s, size_t l)
{
		SQHash h = (SQHash)l;  /* seed */
		size_t step = (l>>5)|1;  /* if string is too long, don't hash all its chars */
		for (; l>=step; l-=step)
			h = h ^ ((h<<5)+(h>>2)+(unsigned short)*(s++));
		return h;
}

template <Squirk T>
struct SQString : public SQRefCounted<T>
{
	SQString(){}
	~SQString(){}
public:
	static SQString *Create(SQSharedState<T> *ss, const SQChar *, SQInteger len = -1 );
	SQInteger Next(const SQObjectPtr<T> &refpos, SQObjectPtr<T> &outkey, SQObjectPtr<T> &outval);
	void Release();
	SQSharedState<T> *_sharedstate;
	SQString<T> *_next; //chain for the string table
	SQInteger _len;

#if defined(_SQ_M2) && defined(_WIN64)
	SQInteger _m2_unknown;
#endif

	SQHash _hash;
	SQChar _val[1];
};

template SQString<Squirk::Standard>;
template SQString<Squirk::AlignObject>;

#endif //_SQSTRING_H_
