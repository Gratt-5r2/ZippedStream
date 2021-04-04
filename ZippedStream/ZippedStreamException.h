#pragma once
#include <exception>
#define ZIPASSERT(e, m) if( !(e) ) throw std::exception( m );