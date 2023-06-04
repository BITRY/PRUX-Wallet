// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"
#include <cmath>
#include "auxpow.h"
#include "arith_uint256.h"
#include "chain.h"
#include "prux.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    int64_t nTargetTimespan = 60 * 6;
    int64_t nTargetSpacing = 3;
    int64_t nInterval = nTargetTimespan / nTargetSpacing;	
    int64_t nReTargetHistoryFact = 2;


     if (pindexLast->nHeight >= 20718300) {
    nTargetTimespan = 2 * 27; 
    nTargetSpacing = 27; 
    nInterval = nTargetTimespan / nTargetSpacing;
    nReTargetHistoryFact = 2; 
    } else if (pindexLast->nHeight >= 20539595) {
    nTargetTimespan = 16 * 27; 
    nTargetSpacing = 27; 
    nInterval = nTargetTimespan / nTargetSpacing;
    nReTargetHistoryFact = 2;  
    } else if (pindexLast->nHeight >= 7770000) {
    nTargetTimespan = 15 * 60; 
    nTargetSpacing = 9; 
    nInterval = nTargetTimespan / nTargetSpacing;
    nReTargetHistoryFact = 2;  
    } else if (pindexLast->nHeight >= 7331700) {
    nTargetTimespan = 5 * 60 * 60; 
    nTargetSpacing = 9; 
    nInterval = nTargetTimespan / nTargetSpacing;
    nReTargetHistoryFact = 6;
    } else {
    nTargetTimespan = 60 * 6; // prux:
    nTargetSpacing = 3; // prux:
    nInterval = nTargetTimespan / nTargetSpacing;
    nReTargetHistoryFact = 2;
    }


    // Only change once per interval
    if ((pindexLast->nHeight+1) % nInterval != 0)
    {
        // Special difficulty rule for testnet:
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->nTime > pindexLast->nTime + nTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % nInterval != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
		
        return pindexLast->nBits;
    }
	
    // Fastcoin: This fixes an issue where a 51% attack can change difficulty at will.
    // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
    int blockstogoback = nInterval-1;
    if ((pindexLast->nHeight+1) != nInterval)
        blockstogoback = nInterval;
    if (pindexLast->nHeight > 15000) 
        blockstogoback = nReTargetHistoryFact * nInterval;
	
    // Go back by what we want to be 14 days worth of blocks
    int nHeightFirst = pindexLast->nHeight - blockstogoback;
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    return CalculatePruxNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{

int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
int64_t nReTargetHistoryFact = 2;
int64_t nTargetTimespan = 60 * 6; // PruxCoin: 1 hours

        if (pindexLast->nHeight >= 20718300) {
        nReTargetHistoryFact = 2;
        nTargetTimespan = 2 * 27;
    } else if (pindexLast->nHeight >= 20539595) {
        nReTargetHistoryFact = 2;
        nTargetTimespan = 16 * 27;
    } else if (pindexLast->nHeight >= 7770000) {
        nReTargetHistoryFact = 2;
        nTargetTimespan = 15 * 60;
    } else if (pindexLast->nHeight >= 7331700) {
        nReTargetHistoryFact = 6;
        nTargetTimespan = 5 * 60 * 60;
    } else {
        nReTargetHistoryFact = 2;
        nTargetTimespan = 60 * 6;
    }

   if (pindexLast->nHeight > 15000)
        // obtain average actual timespan
        nActualTimespan = (pindexLast->GetBlockTime() - nFirstBlockTime)/nReTargetHistoryFact;
    else
        nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    
    if (nActualTimespan < nTargetTimespan/4)
        nActualTimespan = nTargetTimespan/4;
    if (nActualTimespan > nTargetTimespan*4)
        nActualTimespan = nTargetTimespan*4;
	
    // Retarget
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;
	

const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
if (bnNew > bnPowLimit)
    bnNew = bnPowLimit;


return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
