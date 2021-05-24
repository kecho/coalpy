#include "testsystem.h"

namespace coalpy
{

static ApplicationContext g_ctx = {};

const ApplicationContext& ApplicationContext::get()
{
    return g_ctx;
}

void ApplicationContext::set(const ApplicationContext& ctx)
{
    g_ctx = ctx;
}

}
