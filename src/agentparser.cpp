#include "agentparser.h"

/**
 * @brief Regular expression to validate User-Agent HTTP header values.
 *
 * Direct conversion from the syntax in RFC 9110 (HTTP Semantics) and
 * definitions in RFC 5234 (Augmented BNF for Syntax Specifications).
 *
 * -- Extracted syntax rules --
 * VCHAR           =  %x21-7E
 * DIGIT           =  %x30-39
 * ALPHA           =  %x41-5A / %x61-7A
 * SP              =  %x20
 * HTAB            =  %x09
 * WSP             =  SP / HTAB
 * RWS             =  1*( SP / HTAB )
 * tchar           =  "!" / "#" / "$" / "%" / "&" / "'" / "*" /
 *                    "+" / "-" / "." / "^" / "_" / "`" / "|" / "~" /
 *                    DIGIT / ALPHA
 * token           =  1*tchar
 * obs-text        =  %x80-FF
 * quoted-pair     =  "\" ( HTAB / SP / VCHAR / obs-text )
 * ctext           =  HTAB / SP / %x21-27 / %x2A-5B / %x5D-7E / obs-text
 * comment         =  "(" *( ctext / quoted-pair / comment ) ")"
 * product-version =  token
 * product         =  token ["/" product-version]
 * User-Agent      =  product *( RWS ( product / comment ) )
 *
 * -- Named capture-groups --
 * VCHAR           :  (?<vchar>[\x21-\x7e])
 * DIGIT           :  (?<digit>[0-9])
 * ALPHA           :  (?<alpha>[A-Za-z])
 * SP              :  (?<sp>\x20)
 * HTAB            :  (?<htab>\x09)
 * WSP             :  (?<wsp>\g<sp>|\g<htab>)
 * RWS             :  (?<rws>(\g<sp>|\g<htab>)+)
 * tchar           :  (?<tchar>[!#$%&'*+\-.^_`|~]|\g<digit>|\g<alpha>)
 * token           :  (?<token>\g<tchar>+)
 * obs-text        :  (?<obs_text>[\x80-\xff])
 * quoted-pair     :  (?<quoted_pair>\\(\g<htab>|\g<sp>|\g<vchar>|\g<obs_text>))
 * ctext           :  (?<ctext>\g<htab>|\g<sp>|[\x21-\x27]|[\x2a-\x5b]|[\x5d-\x7e]|\g<obs_text>)
 * comment         :  (?<comment>\((\g<ctext>|\g<quoted_pair>|\g<comment>)*\))
 * product         :  (?<product>\g<token>(\/\g<token>)?)
 * User-Agent      :  \g<product>(\g<rws>(\g<product>|\g<comment>))*
 *
 * -- User-Agent RegEx after substitution --
 * (?<product>(?<token>([!#$%&'*+\-.^_`|~]|[0-9]|[A-Za-z])+)(\/\g<token>)?)((\x20|\x09)+(\g<product>|(?<comment>\((\x09|\x20|[\x21-\x27]|[\x2a-\x5b]|[\x5d-\x7e]|(?<obs_text>[\x80-\xff])|\\(\x09|\x20|[\x21-\x7e]|\g<obs_text>)|\g<comment>)*\))))*
 *
 * -- User-Agent RegEx after optimization --
 * (?<product>(?<token>[!#$%&'*+\-.^_`|~0-9A-Za-z]+)(\/\g<token>)?)([\x09\x20]+(\g<product>|(?<comment>\(([\x09\x20-\x27\x2a-\x5b\x5d-\x7e]|(?<obs_text>[\x80-\xff])|\\([\x09\x20-\x7e]|\g<obs_text>)|\g<comment>)*\))))*
 *
 * @note References: https://www.rfc-editor.org/rfc/rfc9110 and
 *                   https://www.rfc-editor.org/rfc/rfc5234
 * @note See it in execution here:    https://regex101.com/r/AQYL5D/1
 * @note Need samples? Get them here: https://www.useragents.me
 * @note Found a bug? Report it here: quark1482 <at> protonmail.com
 */
#define REGEX_USER_AGENT R"((?<product>(?<token>[!#$%&'*+\-.^_`|~0-9A-Za-z]+)(\/\g<token>)?)([\x09\x20]+(\g<product>|(?<comment>\(([\x09\x20-\x27\x2a-\x5b\x5d-\x7e]|(?<obs_text>[\x80-\xff])|\\([\x09\x20-\x7e]|\g<obs_text>)|\g<comment>)*\))))*)"

bool AgentParser::isUserAgent(QString sAgent) {
    bool                    bResult=false;
    QRegularExpression      rxRegEx(QStringLiteral(REGEX_USER_AGENT));
    QRegularExpressionMatch rxmMatch=rxRegEx.match(sAgent);
    if(rxmMatch.hasMatch())
        bResult=sAgent==rxmMatch.captured(0);
    return bResult;
}
