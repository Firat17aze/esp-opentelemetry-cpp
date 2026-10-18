// Replaces opentelemetry-cpp's sdk/src/metrics/instrument_metadata_validator.cc.
// Upstream matches names and units with std::regex, and libstdc++'s matcher
// recurses once per input character, so a long name overflows a task stack of
// a few KB. These loops accept exactly what the specification's patterns
// accept, in constant stack:
//   instrument-name = ALPHA 0*254 ("_" / "." / "-" / "/" / ALPHA / DIGIT)
//   instrument-unit = 0*63 of %x01-7F
// They check the whole view, so an embedded NUL is rejected; the regex read the
// view as a C string and stopped at the NUL.

#include "opentelemetry/sdk/metrics/instrument_metadata_validator.h"

#include <cstddef>

#include "opentelemetry/nostd/string_view.h"
#include "opentelemetry/version.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace sdk
{
namespace metrics
{
namespace
{

constexpr std::size_t kMaxNameSize = 255;
constexpr std::size_t kMaxUnitSize = 63;

// std::isalpha is locale-dependent; the specification's ALPHA is ASCII only.
bool IsAsciiLetter(unsigned char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool IsAsciiDigit(unsigned char c)
{
  return c >= '0' && c <= '9';
}

bool IsNameChar(unsigned char c)
{
  return IsAsciiLetter(c) || IsAsciiDigit(c) || c == '_' || c == '.' || c == '-' || c == '/';
}

bool IsUnitChar(unsigned char c)
{
  return c >= 0x01 && c <= 0x7F;
}

}  // namespace

// The header's regex members stay default-constructed: nothing reads them.
InstrumentMetaDataValidator::InstrumentMetaDataValidator() = default;

bool InstrumentMetaDataValidator::ValidateName(nostd::string_view name) const
{
  if (name.empty() || name.size() > kMaxNameSize ||
      !IsAsciiLetter(static_cast<unsigned char>(name[0])))
  {
    return false;
  }
  for (std::size_t i = 1; i < name.size(); ++i)
  {
    if (!IsNameChar(static_cast<unsigned char>(name[i])))
    {
      return false;
    }
  }
  return true;
}

bool InstrumentMetaDataValidator::ValidateUnit(nostd::string_view unit) const
{
  if (unit.size() > kMaxUnitSize)
  {
    return false;
  }
  for (const char c : unit)
  {
    if (!IsUnitChar(static_cast<unsigned char>(c)))
    {
      return false;
    }
  }
  return true;
}

bool InstrumentMetaDataValidator::ValidateDescription(nostd::string_view /*description*/) const
{
  return true;
}

}  // namespace metrics
}  // namespace sdk
OPENTELEMETRY_END_NAMESPACE
