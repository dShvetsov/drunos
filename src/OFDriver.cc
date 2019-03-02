#include "OFDriver.hh"

#include "oxm/field_set.hh"
#include "types/exception.hh"
#include "SwitchConnection.hh"

#include "Common.hh"
#include "FluidOXMAdapter.hh"


namespace runos {
namespace {

ActionSet convert_to_action_set(const Actions& acts) {
    ActionSet ret;
    for (const oxm::field<>& f : acts.set_fields) {
        ret.add_action(new of13::SetFieldAction(new FluidOXMAdapter(f)));
    }
    if (acts.out_port != 0) {
        ret.add_action(new of13::OutputAction(acts.out_port, 0));// OFP_NO_BUFFER));
    }
    if (acts.group_id != 0) {
        ret.add_action(new of13::GroupAction(acts.group_id));
    }
    return ret;
}

of13::ApplyActions convert_to_apply_action(const Actions& acts) {
    of13::ApplyActions ret;
    for (const oxm::field<>& f : acts.set_fields) {
        ret.add_action(new of13::SetFieldAction(new FluidOXMAdapter(f)));
    }
    if (acts.out_port != 0) {
        ret.add_action(new of13::OutputAction(acts.out_port, 0)); //OFP_NO_BUFFER));
    }
    if (acts.group_id != 0) {
        ret.add_action(new of13::GroupAction(acts.group_id));
    }
    return ret;
}

class Fluid13Rule: public Rule {
public:
    Fluid13Rule(
        SwitchConnectionPtr conn,
        oxm::field_set match,
        uint8_t table,
        uint16_t prio,
        Actions acts,
        uint64_t cookie
    ) : m_conn(conn)
      , m_match(match)
      , m_table(table)
      , m_prio(prio)
      , m_acts(acts)
      , m_cookie(cookie)
    {
        if (m_conn) {
            of13::FlowMod fm;
            fm.command(of13::OFPFC_ADD);
            fm.buffer_id(OFP_NO_BUFFER);
            fm.table_id(m_table);
            fm.cookie(m_cookie);
            fm.match(make_of_match(m_match));
            fm.flags( of13::OFPFF_CHECK_OVERLAP |
                      of13::OFPFF_SEND_FLOW_REM );
            of13::ApplyActions apply_actions = convert_to_apply_action(m_acts);
            fm.add_instruction(apply_actions);
            m_conn->send(fm);
        }
    }

    ~Fluid13Rule() {
        if (m_conn) {
            of13::FlowMod fm;
            fm.command(of13::OFPFC_DELETE);
            fm.cookie(m_cookie);
            fm.cookie_mask(0xfffffffff);
            fm.out_port(of13::OFPP_ANY);
            fm.out_group(of13::OFPG_ANY);
            m_conn->send(fm);
        }
    }
private:
    SwitchConnectionPtr m_conn;
    oxm::field_set m_match;
    uint8_t m_table;
    uint16_t m_prio;
    Actions m_acts;
    uint64_t m_cookie;
};

class Fluid13Group: public Group {
public:
    Fluid13Group(SwitchConnectionPtr conn, uint32_t id, GroupType type, std::vector<Actions> buckets)
        : m_id(id)
        , m_conn(conn)
        , m_type(type)
        , m_buckets(std::move(buckets))
    {
        if (type != GroupType::All) {
            RUNOS_THROW(runtime_error{}); // "Only ALL Type Supported");
        }
        if (m_conn) {
            of13::GroupMod gm;
            gm.commmand(of13::OFPGC_ADD);
            gm.group_type(of13::OFPGT_ALL);
            for (auto& acts: m_buckets) {
                LOG(INFO) << "  Bucket!";
                of13::Bucket b;
                b.watch_port(of13::OFPP_ANY);
                b.watch_group(of13::OFPG_ANY);
                ActionSet action_set = convert_to_action_set(acts);
                b.actions(action_set);
                gm.add_bucket(b);
            }
            gm.group_id(m_id);
            m_conn->send(gm);
        }
    }

    uint32_t id() const override {
        return m_id;
    }

    ~Fluid13Group() {
        if (m_conn) {
            of13::GroupMod gm;
            gm.commmand(of13::OFPGC_DELETE);
            gm.group_type(of13::OFPGT_ALL);
            gm.group_id(m_id);
            m_conn->send(gm);
        }

    }

private:
    uint32_t m_id;
    SwitchConnectionPtr m_conn;
    GroupType m_type;
    std::vector<Actions> m_buckets;
};

// TODO: Move to another file
class Fluid13Driver: public OFDriver {
public:
    Fluid13Driver(SwitchConnectionPtr conn)
        : m_conn(conn)
    { }

    RulePtr installRule(oxm::field_set match, uint16_t prio, Actions actions, uint8_t table) override {

        LOG(INFO) << "Install rule with cookie: " << std::hex << m_cookie_gen;

        RulePtr ret = std::make_shared<Fluid13Rule>(
            m_conn,
            match,
            table,
            prio,
            actions,
            m_cookie_gen
        );
        m_cookie_gen++;
        return ret;
    }
    GroupPtr installGroup(GroupType type, std::vector<Actions> buckets) override {

        LOG(INFO) << "Install group with id: " << std::hex << m_id_gen;

        GroupPtr ret = std::make_shared<Fluid13Group>(
            m_conn,
            m_id_gen,
            type,
            buckets
        );
        m_id_gen++;
        return ret;
    }
private:
    SwitchConnectionPtr m_conn;
    uint16_t m_id_gen = 630;
    uint64_t m_cookie_gen = 0x400000000;
};

} // namespace anon

OFDriverPtr makeDriver(SwitchConnectionPtr conn) {
    return std::make_shared<Fluid13Driver>(conn);
}

} // namespace runos
