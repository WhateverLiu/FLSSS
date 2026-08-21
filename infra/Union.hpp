#pragma once

auto Union ( auto && x, auto && y) {
  return remove_duplicates(append(x, y));
}

