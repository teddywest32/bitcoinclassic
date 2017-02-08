#ifndef TXORPHANCACHE_H
#define TXORPHANCACHE_H

#include "sync.h"
#include "primitives/transaction.h"

class CTxOrphanCache
{
public:
    CTxOrphanCache();
    static CTxOrphanCache *instance();

    struct COrphanTx {
        CTransaction tx;
        int fromPeer;
        uint64_t nEntryTime;
    };
    bool AddOrphanTx(const CTransaction& tx, int peerId);

    void EraseOrphansFor(int peerId);

    void EraseOrphansByTime();

    std::uint32_t LimitOrphanTxSize();

    inline const std::map<uint256, COrphanTx> & mapOrphanTransactions() const {
        return m_mapOrphanTransactions;
    }

    static void clear();
    static bool value(const uint256 &txid, CTransaction &output);
    static bool contains(const uint256 &txid);

    std::vector<uint256> fetchTransactionIds() const;

    void setLimit(std::uint32_t limit);

    std::vector<COrphanTx> fetchTransactionsByPrev(const uint256 &txid) const;

    void EraseOrphans(const std::vector<uint256> &txIds);

protected:
    mutable CCriticalSection m_lock;
    std::map<uint256, COrphanTx> m_mapOrphanTransactions;
    std::map<uint256, std::set<uint256> > m_mapOrphanTransactionsByPrev;

    static CTxOrphanCache *s_instance;

    // this one doesn't lock!
    void EraseOrphanTx(uint256 hash);
    uint32_t LimitOrphanTxSize(uint32_t nMaxOrphans);

private:
    std::uint32_t m_limit;
};

#endif // TXORPHANCACHE_H
