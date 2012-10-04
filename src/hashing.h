#ifndef _HASHING_H_
#define _HASHING_H_

#include "dnasequence.h"

namespace SyntenyFinder
{
	template<class Iterator>
		class SlidingWindow
		{
		public:
			static const int64_t HASH_MOD;
			static const int64_t HASH_BASE;	
			SlidingWindow() {}
			SlidingWindow(Iterator kMerStart, Iterator end, size_t k): k_(k), highPow_(1), 
				kMerStart_(kMerStart), kMerEnd_(AdvanceForward(kMerStart, k - 1)), end_(end)
			{
				for(size_t i = 1; i < k; i++)
				{
					highPow_ = (highPow_ * HASH_BASE) % HASH_MOD;
				}

				value_ = CalcKMerHash(kMerStart, k);
			}

			size_t GetValue() const
			{
				return value_;
			}

			size_t GetK() const
			{
				return k_;
			}

			Iterator GetBegin() const
			{
				return kMerStart_;
			}
			
			Iterator GetEnd() const
			{
				DNASequence::StrandIterator ret = kMerEnd_;
				return ++ret;
			}

			bool Move()
			{
				int64_t sub = (*kMerStart_ * highPow_) % HASH_MOD;
				if(sub <= value_)
				{
					value_ -= sub;
				}
				else
				{
					value_ = HASH_MOD - (sub - value_);
				}

				value_ = (value_ * HASH_BASE) % HASH_MOD;
				++kMerStart_;
				++kMerEnd_;
				if(Valid())
				{
					value_ = (value_ + *kMerEnd_) % HASH_MOD;
					assert(value_ == CalcKMerHash(kMerStart_, k_));
					return true;
				}

				return false;
			}

			bool Valid() const
			{
				return kMerEnd_ != end_;
			}

			static size_t CalcKMerHash(Iterator it, size_t k)
			{
				int64_t base = 1;
				int64_t hash = 0;
				std::advance(it, k - 1);
				for(size_t i = 0; i < k; i++)
				{			
					hash = (hash +  (*it * base) % HASH_MOD) % HASH_MOD;
					base = (base * HASH_BASE) % HASH_MOD;
					if(i != k - 1)
					{
						--it;
					}
				}		

				return hash;
			}

		private:
			int64_t k_;
			int64_t highPow_;
			Iterator kMerStart_;
			Iterator kMerEnd_;
			Iterator end_;
			int64_t value_;
		};

	template<class T>
		const int64_t SlidingWindow<T>::HASH_MOD = 2038076783;

	template<class T>
		const int64_t SlidingWindow<T>::HASH_BASE = 57;
	
	template<class Iterator>
		class KMerHashFunction
		{
		public:
			size_t operator ()(Iterator it) const
			{
				return SlidingWindow<Iterator>::CalcKMerHash(it, k_);
			}

			KMerHashFunction(size_t k): k_(k) {}
		private:
			size_t k_;
		};		
		
	class KMerEqualTo
	{
	public:
		bool operator()(DNASequence::StrandIterator it1, DNASequence::StrandIterator it2) const
		{	
			DNASequence::StrandIterator end1(AdvanceForward(it1, k_));
			return std::mismatch(it1, end1, it2).first == end1;
		}

		KMerEqualTo(size_t k): k_(k) {}
	private:
		size_t k_;
	};
	
	class KMerDumbEqualTo
	{
	public:
		bool operator()(DNASequence::StrandIterator it1, DNASequence::StrandIterator it2) const
		{	
			return true;
		}
	};

	class WindowHashFunction
	{
	public:
		WindowHashFunction(const SlidingWindow<DNASequence::StrandIterator> & window): window_(window) {}

		size_t operator () (DNASequence::StrandIterator it) const
		{
			if(window_.GetBegin() == it)
			{
				return window_.GetValue();
			}

			return SlidingWindow<DNASequence::StrandIterator>::CalcKMerHash(it, window_.GetK());
		}

	private:
		const SlidingWindow<DNASequence::StrandIterator> & window_;
	};
}

#endif