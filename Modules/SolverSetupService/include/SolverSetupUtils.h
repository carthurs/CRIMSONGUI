#pragma once

#include <gsl.h>

#include <QString>

namespace gsl {
    inline QString to_QString(gsl::cstring_span<> view) { return QString::fromLatin1(view.data(), view.size()); }
}