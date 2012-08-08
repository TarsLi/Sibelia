#include "graphalgorithm.h"

namespace SyntenyBuilder
{
	namespace
	{
		struct BifurcationData
		{
		public:
			static const size_t NO_ID;
			static const char NO_CHAR;		
			BifurcationData(size_t id = NO_ID): id_(id), forward_(NO_CHAR), backward_(NO_CHAR) {}
			bool UpdateForward(char nowForward)
			{
				if(id_ == NO_ID && nowForward != NO_CHAR)
				{
					if(forward_ == NO_CHAR)
					{
						forward_ = nowForward;
					}
					else if(forward_ != nowForward)
					{
						return true;
					}
				}

				return false;
			}

			bool UpdateBackward(char nowBackward)
			{
				if(id_ == NO_ID && nowBackward != NO_CHAR)
				{
					if(backward_ == NO_CHAR)
					{
						backward_ = nowBackward;
					}
					else if(backward_ != nowBackward)
					{
						return true;
					}
				}

				return false;
			}
			
			void SetId(size_t newId)
			{
				id_ = newId;
			}

			size_t GetId() const
			{
				return id_;
			}

		private:
			size_t id_;
			char forward_;
			char backward_;
		};

		const size_t BifurcationData::NO_ID = -1;
		const char BifurcationData::NO_CHAR = -1;
	}
	
	/*
#ifdef _DEBUG
	std::map<std::string, size_t> idMap;
	void GraphAlgorithm::Test(const DNASequence & sequence, BifurcationStorage & bifStorage, size_t k)
	{
		IteratorPair it[] = {std::make_pair(sequence.PositiveBegin(), sequence.PositiveEnd()),
			std::make_pair(sequence.NegativeBegin(), sequence.NegativeRightEnd())};
		typedef boost::unordered_map<StrandIterator, size_t, KMerIndex::KMerHashFunction,
			KMerIndex::KMerEqualTo> KMerBifMap;
		KMerBifMap kmerBif(0, KMerIndex::KMerHashFunction(k), KMerIndex::KMerEqualTo(k));
		for(size_t strand = 0; strand < 2; strand++)
		{
			for(StrandIterator jt = it[strand].first; jt != it[strand].second; ++jt)
			{
				if(jt.ProperKMer(k))
				{					
					size_t actualBifurcation = bifStorage.GetBifurcation(jt);
					std::map<std::string, size_t>::iterator kt = 
						idMap.find(std::string(jt, AdvanceForward(jt, k)));
					size_t mustbeBifurcation = kt == idMap.end() ? BifurcationStorage::NO_BIFURCATION :
						kt->second;
					assert(actualBifurcation == mustbeBifurcation);
				}
			}
		}
	}
#endif
	*/

	size_t GraphAlgorithm::EnumerateBifurcations(const DNASequence & sequence, BifurcationStorage & bifStorage, size_t k)
	{
		bifStorage.Clear();
		std::cerr << DELIMITER << std::endl;
		std::cerr << "Finding all bifurcations in the graph..." << std::endl;
		
		const size_t MOD = 1000000;
		size_t bifurcationCount = 0;
		typedef boost::unordered_map<StrandIterator, BifurcationData,
			KMerHashFunction, KMerEqualTo> BifurcationMap;
		BifurcationMap bifurcation(sequence.Size(), KMerHashFunction(k), KMerEqualTo(k));

		StrandIterator border[] = 
		{
			sequence.PositiveBegin(),
			sequence.NegativeBegin(),
			AdvanceBackward(sequence.PositiveEnd(), k),
			AdvanceBackward(sequence.NegativeEnd(), k),
			sequence.PositiveEnd(),
			sequence.NegativeEnd()
		};

		for(size_t i = 0; i < 4; i++)
		{
			bifurcation.insert(std::make_pair(border[i], BifurcationData(bifurcationCount++)));
		}
		
		SlidingWindow<StrandIterator> window(++sequence.PositiveBegin(), --sequence.PositiveEnd(), k); 
		for(size_t count = 0; window.Valid(); window.Move(), count++)
		{
			if(count % MOD == 0)
			{
				std::cerr << "Pos = " << count << std::endl;
			}

			StrandIterator it = window.GetBegin();
			BifurcationMap::iterator jt = bifurcation.find(it);
			if(jt == bifurcation.end())
			{
				bifurcation.insert(std::make_pair(it, BifurcationData()));
			}
			else
			{
				if(jt->second.UpdateForward(*window.GetEnd()) || jt->second.UpdateBackward(*(--it)))
				{
					jt->second.SetId(bifurcationCount++);
				}
			}
		}

		window = SlidingWindow<StrandIterator>(++sequence.NegativeBegin(), --sequence.NegativeEnd(), k); 
		for(size_t count = 0; window.Valid(); window.Move(), count++)
		{
			StrandIterator it = window.GetBegin();
			BifurcationMap::iterator jt = bifurcation.find(it);
			if(jt != bifurcation.end())
			{
				if(jt->second.UpdateForward(*window.GetEnd()) || jt->second.UpdateBackward(*(--it)))
				{
					jt->second.SetId(bifurcationCount++);
				}
			}			
		}

		for(size_t i = 0; i < 2; i++)
		{
			window = SlidingWindow<StrandIterator>(border[i], border[i + 4], k);
			for(size_t count = 0; window.Valid(); window.Move(), count++)
			{
				if(count % MOD == 0)
				{
					std::cerr << "Pos = " << count << std::endl;
				}

				StrandIterator it = window.GetBegin();
				BifurcationMap::iterator jt = bifurcation.find(it);
				if(jt != bifurcation.end() && jt->second.GetId() != BifurcationData::NO_ID)
				{
					bifStorage.AddPoint(it, jt->second.GetId());
				}			
			}	
		}

	#ifdef _DEBUG				
		for(BifurcationMap::iterator it = bifurcation.begin(); it != bifurcation.end(); ++it)
		{
			if(it->second.GetId() != BifurcationData::NO_ID)
			{
				std::cerr << "Id = " << it->second.GetId() << std::endl << "Body = ";
				CopyN(it->first, k, std::ostream_iterator<char>(std::cerr));
				std::cerr << std::endl;
			}
		}
	#endif

		return bifurcationCount;
	}
/*
	void GraphAlgorithm::FindGraphBulges(const DNASequence & sequence, size_t k)
	{
		size_t totalBulges = 0;
		size_t totalWhirls = 0;
		const size_t MOD = 1000;
		BifurcationStorage bifStorage;
		size_t bifurcationCount = GraphAlgorithm::EnumerateBifurcations(sequence, k, bifStorage);
		std::cerr << "Total bifurcations: " << bifurcationCount << std::endl;
		std::cerr << "Finding bulges..." << std::endl;
		for(size_t id = 0; id < bifurcationCount; id++)
		{
			if(id % MOD == 0)
			{
				std::cout << "id = " << id << std::endl;
			}

			FindBulges(sequence, bifStorage, k, id);
		}
	}

	void GraphAlgorithm::SimplifyGraph(DNASequence & sequence, size_t k, size_t minBranchSize)
	{
		size_t totalBulges = 0;
		size_t totalWhirls = 0;
		size_t iterations = 0;
		const size_t MOD = 1000;
		bool anyChanges = true;
		BifurcationStorage bifStorage;
		size_t bifurcationCount = GraphAlgorithm::EnumerateBifurcations(sequence, k, bifStorage);
		std::cerr << "Total bifurcations: " << bifurcationCount << std::endl;
		do
		{
			totalBulges = 0;
			size_t counter = 0;
			//std::cerr << "Removing whirls..." << std::endl;
			//totalWhirls = RemoveWhirls(bifStorage, sequence, k, minBranchSize);

			std::cerr << "Iteration: " << iterations++ << std::endl;
			std::cerr << "Removing bulges..." << std::endl;
			for(size_t id = 0; id < bifurcationCount; id++)
			{
				if(id % MOD == 0)
				{
					std::cout << "id = " << id << std::endl;
				}

				totalBulges += RemoveBulges(bifStorage, sequence, k, minBranchSize, id);
			}

			//std::cerr << "Total whirls: " << totalWhirls << std::endl;
			std::cerr << "Total bulges: " << totalBulges << std::endl;		
		}
		while((totalBulges > 0) && iterations < 10);
	}*/
}
