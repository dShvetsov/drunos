#pragma once

#include "Application.hh"
#include "Loader.hh"
#include "retic/policies.hh"


class Retic: public Application {
SIMPLE_APPLICATION(Retic, "retic")
public:
    void init(Loader* loader, const Config& config) override;

private:
    runos::retic::policy m_policy = runos::retic::fwd(3);
};
