#pragma once

#include <QStringList>
#include <variant>

#include "assemblererror.h"

namespace Ripes {
namespace AssemblerTmp {

inline int getImmediate(QString string, bool& canConvert) {
    string = string.toUpper();
    canConvert = false;
    int immediate = string.toInt(&canConvert, 10);
    int sign = 1;
    if (!canConvert) {
        // Could not convert directly to integer - try hex or bin. Here, extra care is taken to account for a
        // potential sign, and include this is the range validation
        if (string[0] == '-' || string[0] == '+') {
            sign = string[0] == '-' ? -1 : 1;
            string.remove(0, 1);
        }
        if (string.startsWith(QLatin1String("0X"))) {
            immediate = string.remove("0X").toUInt(&canConvert, 16);
        } else if (string.startsWith(QLatin1String("0B"))) {
            immediate = string.remove("0B").toUInt(&canConvert, 2);
        } else {
            canConvert = false;
        }
    }
    return sign * immediate;
}

/**
 * @brief joinParentheses takes a number of tokens and merges together tokens contained within top-level parentheses.
 * For example:
 * [lw, x10, (B, +, (3*2))(x10)] => [lw, x10, B + 3*2), x10]
 */
inline bool matchedParens(std::vector<QChar>& parensStack, QChar end) {
    if (parensStack.size() == 0) {
        return false;
    }
    const QChar toMatch = parensStack.at(parensStack.size() - 1);
    parensStack.pop_back();
    return (toMatch == '[' && end == ']') || (toMatch == '(' && end == ')');
}

inline std::variant<Error, QStringList> joinParentheses(QStringList& tokens) {
    QStringList outtokens;
    std::vector<QChar> parensStack;

    QString tokenBuffer;
    auto commitBuffer = [&]() {
        if (!tokenBuffer.isEmpty()) {
            outtokens << tokenBuffer;
            tokenBuffer.clear();
        }
    };

    for (const auto& token : tokens) {
        for (const auto& ch : token) {
            switch (ch.unicode()) {
                case '(':
                case '[':
                    if (!parensStack.empty()) {
                        tokenBuffer.append(ch);
                    } else {
                        commitBuffer();
                    }
                    parensStack.push_back(ch);
                    break;
                case ']':
                case ')': {
                    if (matchedParens(parensStack, ch)) {
                        if (parensStack.empty()) {
                            commitBuffer();
                        } else {
                            tokenBuffer.append(ch);
                        }
                    } else {
                        return {Error(-1, "Unmatched parenthesis")};
                    }
                    break;
                }
                default:
                    tokenBuffer.append(ch);
                    break;
            }
        }
        if (parensStack.empty()) {
            commitBuffer();
        }
    }

    if (parensStack.empty()) {
        return outtokens;
    } else {
        return {Error(-1, "Unmatched parenthesis")};
    }
}

}  // namespace AssemblerTmp
}  // namespace Ripes
