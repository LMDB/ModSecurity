/*
 * ModSecurity, http://www.modsecurity.org/
 * Copyright (c) 2015 - 2020 Trustwave Holdings, Inc. (http://www.trustwave.com/)
 *
 * You may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * If any of the files related to licensing are missing or if you have any
 * other questions related to licensing please contact Trustwave Holdings, Inc.
 * directly using the email address security@modsecurity.org.
 *
 */

#include "src/operators/rx.h"

#include <string>
#include <list>
#include <memory>

#include "src/operators/operator.h"
#include "modsecurity/rule.h"
#include "modsecurity/rule_message.h"
#include "src/rule_with_actions.h"


namespace modsecurity {
namespace operators {


bool Rx::init(const std::string &file, std::string *error) {
    if (m_string->containsMacro() == false) {
        m_re = new Regex(m_param);
    }

    return true;
}


bool Rx::evaluate(Transaction *transaction,
    const RuleWithActions *rule,
    const bpstd::string_view &input,
    RuleMessage *ruleMessage) {
    Regex *re;

    if (m_param.empty() && !m_string->containsMacro()) {
        return true;
    }

    if (m_string->containsMacro()) {
        std::string eparam(m_string->evaluate(transaction));
        re = new Regex(eparam);
    } else {
        re = m_re;
    }

    std::vector<Utils::SMatchCapture> captures;
    // FIXME: searchOneMatch should accept string_view.
    re->searchOneMatch(input.c_str(), captures);
    if (rule && rule->hasCapture() && transaction) {
        for (const Utils::SMatchCapture& capture : captures) {
            const std::string capture_substring(input.substr(capture.m_offset,capture.m_length));
            transaction->m_collections.m_tx_collection->storeOrUpdateFirst(
                std::to_string(capture.m_group), capture_substring);
            ms_dbg_a(transaction, 7, "Added regex subexpression TX." +
                std::to_string(capture.m_group) + ": " + capture_substring);
            transaction->m_matched.push_back(capture_substring);
        }
    }

    for (const auto & capture : captures) {
        logOffset(ruleMessage, capture.m_offset, capture.m_length);
    }

    if (m_string->containsMacro()) {
        delete re;
    }

    if (!captures.empty()) {
        return true;
    }

    return false;
}


}  // namespace operators
}  // namespace modsecurity
