#pragma once
#define sock_ensure(func, ...)                \
    switch((func)) {                          \
    case net::sock::ReadWriteResult::Success: \
        break;                                \
    case net::sock::ReadWriteResult::Error:   \
        bail_a("socket read error");          \
    case net::sock::ReadWriteResult::Abort:   \
        error_act;                            \
    }
