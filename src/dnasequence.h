#ifndef _DNA_SEQUENCE_H_
#define _DNA_SEQUENCE_H_

#include "common.h"
#include "indexiterator.h"

#pragma warning(disable:4355)

namespace SyntenyBuilder
{
	class DNASequence
	{
	private:
		struct ReadingStrategy;
	public:			
		class StrandIterator;

		enum Direction
		{
			positive,
			negative
		};

		class StrandIterator: public std::iterator<std::bidirectional_iterator_tag, char, size_t>
		{
		public:
			typedef ReadingStrategy RStrategy;

 			StrandIterator() {}
			StrandIterator(IndexConstIterator it, const RStrategy * rStrategy): it_(it), rStrategy_(rStrategy) {}

			char operator * () const
			{
				return rStrategy_->GetBase(it_);
			}			

			void Jump(size_t count)
			{
				rStrategy_->Jump(it_, count);
			}

			bool ProperKMer(size_t k) const
			{
				StrandIterator temp(*this);
				temp.Jump(k - 1);
				return temp.Valid();
			}

			StrandIterator Invert() const
			{
				return StrandIterator(it_, rStrategy_->Invert());
			}

			void MakeInverted()
			{
				*this = Invert();
			}

			Direction GetDirection()
			{
				return rStrategy_->GetDirection();
			}

			StrandIterator& operator ++ ()
			{
				rStrategy_->MoveForward(it_);
				return *this;
			}

			StrandIterator operator ++ (int)
			{
				StrandIterator ret(it_, rStrategy_);
				rStrategy_->MoveForward(it_);
				return ret;
			}

			StrandIterator& operator -- ()
			{
				rStrategy_->MoveBackward(it_);
				return *this;
			}

			StrandIterator operator -- (int)
			{
				StrandIterator ret(it_, rStrategy_);
				rStrategy_->MoveBackward(it_);
				return ret;
			}

			bool operator == (StrandIterator comp) const
			{
				return it_ == comp.it_; 
			}

			bool operator != (StrandIterator comp) const
			{
				return !(*this == comp);
			}
			
			bool Valid() const
			{
				return it_.Valid();
			}

			size_t GetPosition() const
			{
				return it_.GetPosition();
			}

			const ReadingStrategy* GetStrategy() const
			{
				return rStrategy_;
			}

		private:
			IndexConstIterator it_;
			const RStrategy * rStrategy_;
		};

		DNASequence(const std::string sequence):
			sequence_(sequence),
			original_(sequence),
			positiveReading_(this),
			negativeReading_(this),
			deletions_(0)
		{

		}

		StrandIterator PositiveByIndex(size_t pos) const
		{
			return StrandIterator(IndexConstIterator(sequence_, pos, DELETED_CHARACTER), &positiveReading_);
		}				

		StrandIterator NegativeByIndex(size_t pos) const
		{
			return StrandIterator(IndexConstIterator(sequence_, pos, DELETED_CHARACTER, reverse), &negativeReading_);
		}

		StrandIterator PositiveBegin() const
		{
			return StrandIterator(IndexConstIterator(sequence_, 0, DELETED_CHARACTER), &positiveReading_);
		}

		StrandIterator PositiveRightEnd() const
		{
			return StrandIterator(MakeRightEnd(sequence_, DELETED_CHARACTER), &positiveReading_);
		}

		StrandIterator NegativeBegin() const
		{
			return StrandIterator(IndexConstIterator(sequence_, sequence_.size() - 1, DELETED_CHARACTER, reverse),
				&negativeReading_);
		}

		StrandIterator NegativeRightEnd() const
		{
			return StrandIterator(MakeLeftEnd(sequence_, DELETED_CHARACTER), &negativeReading_);
		}

		template<class Iterator>
			std::pair<size_t, size_t> SpellOriginal(StrandIterator it1, StrandIterator it2, Iterator out) const
			{
				StrandIterator out1(IndexConstIterator(original_, it1.GetPosition(), DELETED_CHARACTER), it1.GetStrategy());
				StrandIterator out2(IndexConstIterator(original_, (--it2).GetPosition(), DELETED_CHARACTER), it2.GetStrategy());
				std::copy(out1, ++out2, out);
				size_t pos1 = out1.GetPosition();
				size_t pos2 = (--out2).GetPosition();
				return std::make_pair(std::min(pos1, pos2), std::max(pos1, pos2) + 1);
			}

		template<class Iterator>
			void CopyN(Iterator start, size_t count, StrandIterator out)
			{				
				for(size_t i = 0; i < count; i++, ++out, ++start)
				{
					sequence_[out.GetPosition()] = out.GetStrategy()->Translate(*start);
				}
			}

		void EraseN(StrandIterator out, size_t count)
		{
			deletions_ += count;
			for(size_t i = 0; i < count; i++, ++out)
			{
				sequence_[out.GetPosition()] = DELETED_CHARACTER;
			}
		}

		template<class Iterator>
			void SpellRaw(Iterator out) const
			{
				std::copy(sequence_.begin(), sequence_.end(), out);
			}

		size_t Size() const
		{
			return sequence_.size();
		}

		char GetRawChar(size_t pos) const
		{
			return sequence_[pos];
		}

		static const std::string alphabet;
	private:
		DISALLOW_COPY_AND_ASSIGN(DNASequence);	
		static const std::string complementary_;
		static const char DELETED_CHARACTER;

		struct ReadingStrategy
		{
		public:
			ReadingStrategy(DNASequence * sequence): sequence_(sequence) {}
			char GetBase(IndexConstIterator it) const
			{
				return Translate(*it);
			}
							
			virtual char Translate(char ch) const = 0;
			virtual Direction GetDirection() const = 0;
			virtual const ReadingStrategy* Invert() const = 0;
			virtual void MoveForward(IndexConstIterator & it) const = 0;
			virtual void MoveBackward(IndexConstIterator & it) const = 0;
			virtual void Jump(IndexConstIterator & it, size_t count) const = 0;
		protected:
			DNASequence * sequence_;
		};			

		struct PositiveReadingStrategy: public ReadingStrategy
		{
			PositiveReadingStrategy(DNASequence * sequence): ReadingStrategy(sequence) {}
			Direction GetDirection() const
			{
				return positive;
			}

			char Translate(char ch) const
			{
				return ch;
			}

			void MoveForward(IndexConstIterator & it) const
			{
				++it;
			}

			void MoveBackward(IndexConstIterator & it) const
			{
				--it;
			}

			const ReadingStrategy* Invert() const
			{
				return &sequence_->negativeReading_;
			}

			void Jump(IndexConstIterator & it, size_t count) const
			{
				if(sequence_->deletions_ == 0)
				{
					it = IndexConstIterator(sequence_->sequence_, 
						std::min(it.GetPosition() + count, sequence_->sequence_.size()),
						sequence_->DELETED_CHARACTER);
				}
				else
				{
					for(size_t i = 0; i < count; i++)
					{
						MoveForward(it);
					}
				}
			}
		};

		struct NegativeReadingStrategy: public ReadingStrategy
		{
			NegativeReadingStrategy(DNASequence * sequence): ReadingStrategy(sequence) {}
			Direction GetDirection() const
			{
				return negative;
			}

			char Translate(char ch) const
			{
				return complementary_[ch];
			}

			void MoveForward(IndexConstIterator & it) const
			{
				--it;
			}

			void MoveBackward(IndexConstIterator & it) const
			{
				++it;
			}

			void Jump(IndexConstIterator & it, size_t count) const
			{
				if(sequence_->deletions_ == 0)
				{
					it = IndexConstIterator(sequence_->sequence_, 
						it.GetPosition() < count ? IndexConstIterator::NPOS : it.GetPosition() - count,
						sequence_->DELETED_CHARACTER,
						reverse);

				}
				else
				{
					for(size_t i = 0; i < count; i++)
					{
						MoveForward(it);
					}
				}
			}

			const ReadingStrategy* Invert() const
			{
				return &sequence_->positiveReading_;
			}
		};
		
		PositiveReadingStrategy positiveReading_;
		NegativeReadingStrategy negativeReading_;

		//Current version of the sequence (after possible simplification)		
		std::string sequence_;
		//Original version of the sequence (before doing any modifications)
		std::string original_;
		size_t deletions_;
	};	
}

#endif
