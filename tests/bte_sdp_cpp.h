#ifndef BTE_TESTS_BTE_SDP_CPP_H
#define BTE_TESTS_BTE_SDP_CPP_H

#include "bte_l2cap_cpp.h"

#include "bt-embedded/services/sdp.h"

#include <functional>
#include <iomanip>
#include <memory>
#include <optional>
#include <string>

namespace Bte {

class SdpClient {
public:
    SdpClient(L2cap &l2cap): m_sdp(bte_sdp_client_new(l2cap.m_l2cap)) {}
    SdpClient(const SdpClient &other): m_sdp(bte_sdp_client_ref(other.m_sdp)) {}
    ~SdpClient() { if (m_sdp) bte_sdp_client_unref(m_sdp); }

    SdpClient &operator=(const SdpClient &other) {
        if (m_sdp) bte_sdp_client_unref(m_sdp);
        m_sdp = bte_sdp_client_ref(other.m_sdp);
        return *this;
    }

    struct ServiceSearchReply {
        uint16_t errorCode;
        uint16_t totalCount;
        bool hasMore;
        std::vector<uint32_t> handles;

        static ServiceSearchReply fromC(const BteSdpServiceSearchReply *r) {
            ServiceSearchReply reply {
                r->error_code,
                r->total_count,
                r->has_more,
            };
            for (int i = 0; i < r->count; i++) {
                reply.handles.push_back(r->handles[i]);
            }
            return reply;
        }

        bool operator==(const ServiceSearchReply &r) const {
            return errorCode == r.errorCode &&
                totalCount == r.totalCount && hasMore == r.hasMore &&
                handles == r.handles;
        }
    };

    using ServiceSearchCb =
        std::function<bool(const ServiceSearchReply &reply)>;
    bool serviceSearchReq(std::span<uint16_t> pattern, uint16_t maxCount,
                          const ServiceSearchCb &cb)
    {
        auto *f = new ServiceSearchCb(cb);
        return bte_sdp_service_search_req_uuid16(
            m_sdp, pattern.data(), pattern.size(), maxCount,
            &SdpClient::Callbacks::serviceSearchReq, f);
    }

    struct ServiceAttrReply {
        uint16_t errorCode;
        Buffer attrListDe;

        static ServiceAttrReply fromC(const BteSdpServiceAttrReply *r) {
            ServiceAttrReply reply { r->error_code, };
            if (r->attr_list_de) {
                uint32_t size = bte_sdp_de_get_total_size(r->attr_list_de);
                reply.attrListDe =
                    Buffer(r->attr_list_de, std::next(r->attr_list_de, size));
            }
            return reply;
        }

        bool operator==(const ServiceAttrReply &r) const {
            return errorCode == r.errorCode && attrListDe == r.attrListDe;
        }
    };

    using ServiceAttrCb =
        std::function<void(const ServiceAttrReply &reply)>;
    bool serviceAttrReq(uint32_t service_record, uint16_t max_count,
                        const uint8_t *id_list, const ServiceAttrCb &cb)
    {
        auto *f = new ServiceAttrCb(cb);
        return bte_sdp_service_attr_req(
            m_sdp, service_record, max_count, id_list,
            &SdpClient::Callbacks::serviceAttrReq, f);
    }

private:
    struct Callbacks {
        static bool serviceSearchReq(
            BteSdpClient *sdp, const BteSdpServiceSearchReply *r, void *d) {
            ServiceSearchCb *cb = static_cast<ServiceSearchCb*>(d);
            bool wantsMore = (*cb)(ServiceSearchReply::fromC(r));
            if (!r->has_more || !wantsMore) {
                delete cb;
            }
            return wantsMore;
        }

        static void serviceAttrReq(
            BteSdpClient *sdp, const BteSdpServiceAttrReply *r, void *d) {
            ServiceAttrCb *cb = static_cast<ServiceAttrCb*>(d);
            (*cb)(ServiceAttrReply::fromC(r));
            delete cb;
        }
    };

    BteSdpClient *m_sdp;
};

} // namespace Bte

inline bool operator==(const BteSdpDeUuid128 &a, const BteSdpDeUuid128 &b)
{
    return memcmp(&a, &b, sizeof(a)) == 0;
}

inline std::ostream &operator<<(std::ostream &os,
                                const Bte::SdpClient::ServiceSearchReply &a)
{
    os << "ServiceSearchReply{ec=" << int(a.errorCode) <<", total=" << int(a.totalCount) <<
        ", hm=" << a.hasMore << ", handles=" << a.handles << "}";
    return os;
}

namespace Bte {

inline void PrintTo(const SdpClient::ServiceSearchReply &v, std::ostream *os)
{
    *os << v;
}

} // namespace Bte

#endif /* BTE_TESTS_BTE_SDP_CPP_H */
