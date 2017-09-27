#include "parser.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

struct DocParam : public Parser::Param {
    DocParam(const char* aDoc) : Param("", aDoc) {}

    virtual void parse(Parser::TokenIter&, Parser::TokenIter) {
        throw std::runtime_error("a DocParam cannot be parsed");
    }

    virtual void write(std::ostream&, const std::string&, char) const {
        throw std::runtime_error("a DocParam cannot be written");
    };
};

template <typename T>
struct ParamImplBase : public Parser::Param {
    ParamImplBase(const char* aKey, const char* aDoc, T aVal) : Param(aKey, aDoc), val(aVal) {}

protected:
    static void testHaveToken(Parser::TokenIter& i, Parser::TokenIter ie) {
        if (i == ie) {
            std::cerr << "unexpected end of input -- missing parameter value\n";
            throw std::runtime_error("unexpected end of input -- missing parameter value");
        }
    }

    // data
    T val;
};

struct FlagParam : public ParamImplBase<bool*> {
    using ParamImplBase::ParamImplBase;
    using ParamImplBase::val;

    virtual void parse(Parser::TokenIter&, Parser::TokenIter) override { *val = true; }

    virtual void write(std::ostream& out, const std::string& name, char sep) const override {
        if (*val) { out << name << sep; }
    }
};

struct IntParam : public ParamImplBase<int*> {
    using ParamImplBase::ParamImplBase;
    using ParamImplBase::val;

    virtual void parse(Parser::TokenIter& i, Parser::TokenIter ie) override {
        testHaveToken(i, ie);
        try {
            *val = std::stoi(*i++);
        } catch (std::exception& e) {
            std::cerr << "failed to read an integer\n";
            throw e;
        }
    }

    virtual void write(std::ostream& out, const std::string& name, char sep) const override {
        out << name << ' ' << *val << sep;
    }
};

struct Uint8Param : public ParamImplBase<uint8_t*> {
    using ParamImplBase::ParamImplBase;
    using ParamImplBase::val;

    virtual void parse(Parser::TokenIter& i, Parser::TokenIter ie) override {
        testHaveToken(i, ie);
        try {
            int got = std::stoi(*i++);

            if (got < 0 || got > 255) {
                std::cerr << got << " is not in range 0..255\n";
                throw std::runtime_error("integer out of range");
            }

            *val = uint8_t(got);
        } catch (std::exception& e) {
            std::cerr << "failed to read an 8-bit unsigned integer\n";
            throw e;
        }
    }

    virtual void write(std::ostream& out, const std::string& name, char sep) const override {
        out << name << ' ' << int(*val) << sep;
    }
};

struct FloatParam : public ParamImplBase<float*> {
    using ParamImplBase::ParamImplBase;
    using ParamImplBase::val;

    virtual void parse(Parser::TokenIter& i, Parser::TokenIter ie) override {
        testHaveToken(i, ie);
        try {
            *val = std::stof(*i++);
        } catch (std::exception& e) {
            std::cerr << "failed to read a floating-point number\n";
            throw e;
        }
    }

    virtual void write(std::ostream& out, const std::string& name, char sep) const override {
        out << name << ' ' << *val << sep;
    }
};

struct StringParam : public ParamImplBase<std::string*> {
    using ParamImplBase::ParamImplBase;
    using ParamImplBase::val;

    virtual void parse(Parser::TokenIter& i, Parser::TokenIter ie) override {
        testHaveToken(i, ie);
        *val = *i++;
    }

    virtual void write(std::ostream& out, const std::string& name, char sep) const override {
        out << name << ' ' << std::quoted(*val) << sep;
    }
};

struct StringListParam : public ParamImplBase<std::vector<std::string>*> {
    using ParamImplBase::ParamImplBase;
    using ParamImplBase::val;

    virtual void parse(Parser::TokenIter& i, Parser::TokenIter ie) override {
        testHaveToken(i, ie);
        val->emplace_back(*i++);
    }

    virtual void write(std::ostream& out, const std::string& name, char sep) const override {
        for (auto& item : *val) { out << name << ' ' << std::quoted(item) << sep; }
    }
};

struct CallbackParam : public ParamImplBase<std::function<void()>> {
    using ParamImplBase::ParamImplBase;
    using ParamImplBase::val;

    virtual void parse(Parser::TokenIter&, Parser::TokenIter) override { val(); }

    virtual void write(std::ostream&, const std::string&, char) const override {
        // do nothing
    }
};

struct CallbackStringParam : public ParamImplBase<std::function<void(const std::string&)>> {
    using ParamImplBase::ParamImplBase;
    using ParamImplBase::val;

    virtual void parse(Parser::TokenIter& i, Parser::TokenIter ie) override {
        testHaveToken(i, ie);
        val(*i++);
    }

    virtual void write(std::ostream&, const std::string&, char) const override {
        // do nothing
    }
};

void Parser::addParam(const char* key, Parser::Param* param) {
    mList.emplace_back(param);
    mMap.emplace(std::pair<std::string, Param*>(key, mList.back().get()));
}

void Parser::add(const char* doc) { mList.emplace_back(new DocParam(doc)); }

void Parser::add(const char* key, const char* doc, bool& val) {
    addParam(key, new FlagParam(key, doc, &val));
}

void Parser::add(const char* key, const char* doc, int& val) {
    addParam(key, new IntParam(key, doc, &val));
}

void Parser::add(const char* key, const char* doc, uint8_t& val) {
    addParam(key, new Uint8Param(key, doc, &val));
}

void Parser::add(const char* key, const char* doc, float& val) {
    addParam(key, new FloatParam(key, doc, &val));
}

void Parser::add(const char* key, const char* doc, std::string& val) {
    addParam(key, new StringParam(key, doc, &val));
}

void Parser::add(const char* key, const char* doc, std::vector<std::string>& val) {
    addParam(key, new StringListParam(key, doc, &val));
}

void Parser::add(const char* key, const char* doc, std::function<void()> callback) {
    addParam(key, new CallbackParam(key, doc, callback));
}

void Parser::add(const char* key, const char* doc,
                 std::function<void(const std::string&)> callback) {
    addParam(key, new CallbackStringParam(key, doc, callback));
}

void Parser::parse(const std::string& filename) {
    static constexpr int MAX_FILES = 32;

    if (mNumFilesParsed++ == MAX_FILES) {
        // die when too many files have been parsed to avoid potential infinite loops
        std::cerr << "exceeded the maximum of " << MAX_FILES << " files parsed in one run\n";
        throw std::runtime_error("too many files parsed");
    }

    std::vector<std::string> tokens;
    {
        std::ifstream in{filename};

        if (!in) {
            std::cerr << "failed to open parameter file " << filename << "\n";
            throw std::runtime_error("failed to open file");
        }

        std::string token;
        while (in >> std::quoted(token)) { tokens.emplace_back(token); }
    }

    try {
        parse(tokens);
    } catch (std::exception& e) {
        std::cerr << "while parsing parameters from file '" << filename << "'\n";
        throw e;
    }
}

void Parser::parse(int argc, char** argv) {
    std::vector<std::string> tokens(argv + 1, argv + argc);

    try {
        parse(tokens);
    } catch (std::exception& e) {
        std::cerr << "while parsing parameters from command line\n";
        throw e;
    }
}

void Parser::parse(const std::vector<std::string>& tokens) {
    auto i = begin(tokens);
    auto ie = end(tokens);
    std::string key;

    while (i != ie) {
        key = *i++;
        auto found = mMap.find(key);

        if (found == end(mMap)) {
            std::cerr << "unrecognized parameter '" << key << "'\n";
            throw std::runtime_error("failed to parse parameter");
        }

        try {
            auto& param = found->second;
            param->parse(i, ie);
        } catch (std::exception& e) {
            std::cerr << "while parsing parameter '" << key << "'\n";
            throw e;
        }
    }
}

void Parser::printHelp(std::ostream& out) const {
    static constexpr int COLUMNS = 80;
    static constexpr int MAX_PAD = 11;

    int maxKeyLen = 0;
    for (auto& entry : mMap) { maxKeyLen = std::max(maxKeyLen, int(entry.first.size())); }
    const int pad = std::min(MAX_PAD, maxKeyLen);
    const int maxWidth = COLUMNS - pad;
    std::string key;
    std::string word;
    std::vector<std::string> tokens;

    for (auto& param : mList) {
        // params without a key: just print doc
        key.assign(param->key);
        if (key.empty()) {
            out << param->doc << '\n';
            continue;
        }

        // write key name
        out << key;
        int spaces = pad - int(key.size());
        for (int i = 0; i < spaces; i++) { out << ' '; }
        int widthRemaining = COLUMNS - std::max(pad, int(key.size()));

        // split doc string into words
        std::istringstream iss(mMap.at(key)->doc);
        tokens.clear();
        while (iss >> word) { tokens.push_back(word); }

        // put words
        for (auto& token : tokens) {
            int len = int(token.size()) + 1;

            // break line if out of space
            if (len > widthRemaining) {
                out << '\n';
                for (int i = 0; i < pad; i++) { out << ' '; }
                widthRemaining = maxWidth;
            }

            out << ' ' << token;
            widthRemaining -= len;
        }

        // endline
        out << '\n';
    }
}

void Parser::printValues(std::ostream& out, char sep) const {
    std::vector<std::string> keys;
    for (auto& entry : mMap) { keys.push_back(entry.first); }
    std::sort(begin(keys), end(keys));
    for (auto& key : keys) { mMap.at(key)->write(out, key, sep); }
}
