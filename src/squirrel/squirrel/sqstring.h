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

template <Squirk Q>
struct SQString : public SQRefCounted<Q>
{
	SQString(){}
	~SQString(){}
public:
	static SQString *Create(SQSharedState<Q> *ss, const SQChar *, SQInteger len = -1 );
	SQInteger Next(const SQObjectPtr<Q> &refpos, SQObjectPtr<Q> &outkey, SQObjectPtr<Q> &outval);
	void Release();
	SQSharedState<Q> *_sharedstate;
	SQString<Q> *_next; //chain for the string table
	SQInteger _len;

#if defined(_SQ_M2) && defined(_WIN64)
	SQInteger _m2_unknown;
#endif

	SQHash _hash;
	SQChar _val[1];
};

template SQString<Squirk::Standard>;
template SQString<Squirk::AlignObject>;
template SQString<Squirk::StandardShared>;
template SQString<Squirk::AlignObjectShared>;

#endif //_SQSTRING_H_
