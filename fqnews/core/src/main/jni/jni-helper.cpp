/*******************************************************************************
 *                                                                             *
 *  Copyright (C) 2019 by Max Lv <max.c.lv@gmail.com>                          *
 *  Copyright (C) 2019 by Mygod Studio <contact-shadowsocks-android@mygod.be>  *
 *                                                                             *
 *  This program is free software: you can redistribute it and/or modify       *
 *  it under the terms of the GNU General Public License as published by       *
 *  the Free Software Foundation, either version 3 of the License, or          *
 *  (at your option) any later version.                                        *
 *                                                                             *
 *  This program is distributed in the hope that it will be useful,            *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 *  GNU General Public License for more details.                               *
 *                                                                             *
 *  You should have received a copy of the GNU General Public License          *
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <sstream>

#include "jni.h"
#include "re2/re2.h"

using namespace std;

struct AclMatcher {
    stringstream bypassDomainsBuilder, proxyDomainsBuilder;
    RE2 *bypassDomains, *proxyDomains;

    ~AclMatcher() {
        if (bypassDomains) delete bypassDomains;
        if (proxyDomains) delete proxyDomains;
    }
};

bool addDomain(JNIEnv *env, stringstream &domains, jstring regex) {
    const char *regexChars = env->GetStringUTFChars(regex, nullptr);
    if (regexChars == nullptr) return false;
    if (domains.rdbuf()->in_avail()) domains << '|';
    domains << regexChars;
    env->ReleaseStringUTFChars(regex, regexChars);
    return true;
}

const char *buildRE2(stringstream &domains, RE2 *&out, const RE2::Options &options) {
    if (domains.rdbuf()->in_avail()) {
        out = new RE2(domains.str(), options);
        domains.clear();
        if (!out->ok()) return out->error().c_str();
    } else {
        delete out;
        out = nullptr;
    }
    return nullptr;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
extern "C" {
JNIEXPORT jlong JNICALL Java_com_github_shadowsocks_acl_AclMatcher_init(JNIEnv *env, jclass clazz) {
    return reinterpret_cast<jlong>(new AclMatcher());
}

JNIEXPORT void JNICALL
Java_com_github_shadowsocks_acl_AclMatcher_close(JNIEnv *env, jclass clazz, jlong handle) {
    delete reinterpret_cast<AclMatcher *>(handle);
}

JNIEXPORT jboolean JNICALL
Java_com_github_shadowsocks_acl_AclMatcher_addBypassDomain(JNIEnv *env, jclass clazz, jlong handle,
                                                           jstring regex) {
    return static_cast<jboolean>(handle &&
                                 ::addDomain(env, reinterpret_cast<AclMatcher *>(handle)->bypassDomainsBuilder, regex));
}

JNIEXPORT jboolean JNICALL
Java_com_github_shadowsocks_acl_AclMatcher_addProxyDomain(JNIEnv *env, jclass clazz, jlong handle,
                                                          jstring regex) {
    return static_cast<jboolean>(handle &&
                                 ::addDomain(env, reinterpret_cast<AclMatcher *>(handle)->proxyDomainsBuilder, regex));
}

JNIEXPORT jstring JNICALL
Java_com_github_shadowsocks_acl_AclMatcher_build(JNIEnv *env, jclass clazz, jlong handle,
                                                 jlong memory_limit) {
    if (!handle) return env->NewStringUTF("AclMatcher closed");
    auto matcher = reinterpret_cast<AclMatcher *>(handle);
    RE2::Options options;
    options.set_max_mem(memory_limit);
    options.set_never_capture(true);
    const char *e = ::buildRE2(matcher->bypassDomainsBuilder, matcher->bypassDomains, options);
    if (e) return env->NewStringUTF(e);
    e = ::buildRE2(matcher->proxyDomainsBuilder, matcher->proxyDomains, options);
    if (e) return env->NewStringUTF(e);
    return nullptr;
}

JNIEXPORT jint JNICALL
Java_com_github_shadowsocks_acl_AclMatcher_matchHost(JNIEnv *env, jclass clazz, jlong handle,
                                                     jstring host) {
    if (!handle) return -1;
    auto matcher = reinterpret_cast<const AclMatcher *>(handle);
    const char *hostChars = env->GetStringUTFChars(host, nullptr);
    jint result = 0;
    if (matcher->bypassDomains && RE2::PartialMatch(hostChars, *matcher->bypassDomains)) result = 1;
    else if (matcher->proxyDomains && RE2::PartialMatch(hostChars, *matcher->proxyDomains)) result = 2;
    env->ReleaseStringUTFChars(host, hostChars);
    return result;
}
}
