#include <cassert>
#include <unordered_map>

#include <fstream>

struct CallStackEntry
{
    std::string caller;
    std::string callee;

    bool operator == (const CallStackEntry& callStackEntry) const
    {
        return caller == callStackEntry.caller && callee == callStackEntry.callee;
    }
};

// Required to use CallStackEntry in hash set
namespace std
{
  template<>
    struct hash<CallStackEntry>
    {
      size_t operator()(const CallStackEntry& obj) const
      {
          return hash<string>()(obj.caller) ^ hash<string>()(obj.callee);
      }
    };
} // namespace std

class ShadowCallStack
{
public:
public:
    static ShadowCallStack& getShadowStackInstance()
    {
        static ShadowCallStack shadowStackInstance;
        return shadowStackInstance;
    }

public:
    ShadowCallStack() = default;
    ShadowCallStack(const ShadowCallStack& ) = delete;
    ShadowCallStack(ShadowCallStack&& ) = delete;
    ShadowCallStack& operator =(const ShadowCallStack& ) = delete;
    ShadowCallStack& operator =(ShadowCallStack&& ) = delete;

    ~ShadowCallStack()
    {
        dumpShadowCallStack();
    }

public:
    void addCaller(const std::string& caller)
    {
        assert(m_currentCaller.empty());
        m_currentCaller = caller;
    }

    void addCallee(const std::string& callee)
    {
        assert(!m_currentCaller.empty());
        m_callStackEntryNum[CallStackEntry{m_currentCaller, callee}] += 1;
        m_currentCaller.clear();
    }


    void dumpShadowCallStack()
    {
        std::ofstream strm("shadow_call_stack.txt");
        for (const auto& [callEntry, num] : m_callStackEntryNum) {
            strm << "(" << callEntry.caller << " " << callEntry.callee << ") " << num << "\n";
        }
        strm.flush();
        strm.close();
    }

private:
    std::unordered_map<CallStackEntry, int> m_callStackEntryNum;
    std::string m_currentCaller;
}; // class ShadowCallStack


extern "C" {

void addCaller(const char* caller)
{
    ShadowCallStack::getShadowStackInstance().addCaller(caller);
}

void addCallee(const char* callee)
{
    ShadowCallStack::getShadowStackInstance().addCallee(callee);
}

}

