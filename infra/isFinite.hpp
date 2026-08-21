#pragma once
/**
 * Check if a floating-point number is finite.
 * 
 * This function shall not be replaced by 
 * <a href="https://en.cppreference.com/w/cpp/numeric/math/isfinite">std::isfinite</a> because
 * the library may be compiled with 
 * <a href="https://stackoverflow.com/questions/7420665/what-does-gccs-ffast-math-actually-do">-ffast-math</a>.
 * 
 * @param y  A float or double.
 * 
 * @return A boolean. `True` means the number is finite.
 */
bool isFinite (auto y)
{
  if constexpr (std::is_same<decltype(y), float>::value) {
    constexpr uint32_t a = 0b01111111100000000000000000000000;
    uint32_t x; std::memcpy(&x, &y, sizeof(uint32_t));
    return (x & a) != a; // If input type is float, check like this.
  }
  else if constexpr ( std::is_same<decltype(y), double>::value ) {
    constexpr uint64_t a = 
      0b0111111111110000000000000000000000000000000000000000000000000000;
    uint64_t x; std::memcpy(&x, &y, sizeof(uint64_t));
    return (x & a) != a; // If input type is double, check like this.
  } 
  else static_assert(sizeof(y) == 0,
    "Type is not float or double");
} 



