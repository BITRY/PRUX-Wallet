// Copyright (c) 2015 The Prux Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/random/uniform_int.hpp>
#include <boost/random/mersenne_twister.hpp>

#include "policy/policy.h"
#include "arith_uint256.h"
#include "prux.h"
#include "txmempool.h"
#include "util.h"
#include "validation.h"


unsigned int CalculatePruxNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{ 
int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
int64_t nReTargetHistoryFact = 6;
int64_t nTargetTimespan = 60 * 6; 

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


bool CheckAuxPowProofOfWork(const CBlockHeader& block, const Consensus::Params& params)
{
    /* Except for legacy blocks with full version 1, ensure that
       the chain ID is correct.  Legacy blocks are not allowed since
       the merge-mining start, which is checked in AcceptBlockHeader
       where the height is known.  */
    if (!block.IsLegacy() && params.fStrictChainId && block.GetChainId() != params.nAuxpowChainId)
        return error("%s : block does not have our chain ID"
                     " (got %d, expected %d, full nVersion %d)",
                     __func__, block.GetChainId(),
                     params.nAuxpowChainId, block.nVersion);

    /* If there is no auxpow, just check the block hash.  */
    if (!block.auxpow) {
        if (block.IsAuxpow())
            return error("%s : no auxpow on block with auxpow version",
                         __func__);

        if (!CheckProofOfWork(block.GetPoWHash(), block.nBits, params))
            return error("%s : non-AUX proof of work failed", __func__);

        return true;
    }

    /* We have auxpow.  Check it.  */

    if (!block.IsAuxpow())
        return error("%s : auxpow on block with non-auxpow version", __func__);

    if (!block.auxpow->check(block.GetHash(), block.GetChainId(), params))
        return error("%s : AUX POW is not valid", __func__);
    if (!CheckProofOfWork(block.auxpow->getParentBlockPoWHash(), block.nBits, params))
        return error("%s : AUX proof of work failed", __func__);

    return true;
}

CAmount GetPruxBlockSubsidy(int nHeight, const Consensus::Params& consensusParams, uint256 prevHash)
{
    
    CAmount nSubsidy = 0.009595 * COIN; 
   
    // Subsidy is cut in half every 210,000 blocks which will occur approximately every 4 years.
    nSubsidy >>= (nHeight / 5959595);
    return nSubsidy;
    

}

CAmount GetPruxMinRelayFee(const CTransaction& tx, unsigned int nBytes, bool fAllowFree)
{
    {
        LOCK(mempool.cs);
        uint256 hash = tx.GetHash();
        double dPriorityDelta = 0;
        CAmount nFeeDelta = 0;
        mempool.ApplyDeltas(hash, dPriorityDelta, nFeeDelta);
        if (dPriorityDelta > 0 || nFeeDelta > 0)
            return 0;
    }

    CAmount nMinFee = ::minRelayTxFee.GetFee(nBytes);
    nMinFee += GetPruxDustFee(tx.vout, ::minRelayTxFee);

    if (fAllowFree)
    {
        // There is a free transaction area in blocks created by most miners,
        // * If we are relaying we allow transactions up to DEFAULT_BLOCK_PRIORITY_SIZE - 1000
        //   to be considered to fall into this category. We don't want to encourage sending
        //   multiple transactions instead of one big transaction to avoid fees.
        if (nBytes < (DEFAULT_BLOCK_PRIORITY_SIZE - 1000))
            nMinFee = 0;
    }

    if (!MoneyRange(nMinFee))
        nMinFee = MAX_MONEY;
    return nMinFee;
}

CAmount GetPruxDustFee(const std::vector<CTxOut> &vout, CFeeRate &baseFeeRate) {
    CAmount nFee = 0;

    // To limit dust spam, add base fee for each output less than a COIN
    BOOST_FOREACH(const CTxOut& txout, vout)
        if (txout.IsDust(::minRelayTxFee))
            nFee += baseFeeRate.GetFeePerK();

    return nFee;
}
