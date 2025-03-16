#ifndef A5AC6D2A_8BE7_48ED_AC8E_2A6B66E6A323
#define A5AC6D2A_8BE7_48ED_AC8E_2A6B66E6A323

namespace Utils
{
    template<typename  T>
    static inline T MAX(const T& a, const T& b)
    {
        return (a > b) ? a : b;
    }

    template<typename  T>
    static inline T MIN(const T& a, const T& b)
    {
        return (a < b) ? a : b;
    }

};

#endif /* A5AC6D2A_8BE7_48ED_AC8E_2A6B66E6A323 */
