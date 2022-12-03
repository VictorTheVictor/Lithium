# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'libjingle_source': "source",
    'webrtc_base': "../webrtc/base",
    'webrtc_xmllite': "../webrtc/libjingle/xmllite",
    'webrtc_p2p': "../webrtc/p2p",
    'webrtc_system_wrappers': "../webrtc/system_wrappers",
  },
  'includes': [
    '../../native_client/build/untrusted.gypi',
  ],
  'targets': [
    {
      'target_name': 'libjingle_nacl',
      'type': 'none',
      'variables': {
        'nlib_target': 'libjingle_nacl.a',
        'nacl_untrusted_build': 1,
        'build_glibc': 0,
        'build_newlib': 0,
        'build_pnacl_newlib': 1,
        'use_openssl': 1,
      },
      'dependencies': [
        '<(DEPTH)/native_client_sdk/native_client_sdk_untrusted.gyp:nacl_io_untrusted',
        '<(DEPTH)/third_party/expat/expat_nacl.gyp:expat_nacl',
        '<(DEPTH)/third_party/boringssl/boringssl_nacl.gyp:boringssl_nacl',
        'libjingle_p2p_constants_nacl',
      ],
      'defines': [
        'EXPAT_RELATIVE_PATH',
        'FEATURE_ENABLE_SSL',
        'GTEST_RELATIVE_PATH',
        'HAVE_OPENSSL_SSL_H',
        'NO_MAIN_THREAD_WRAPPING',
        'NO_SOUND_SYSTEM',
        'WEBRTC_POSIX',
        'SRTP_RELATIVE_PATH',
        'SSL_USE_OPENSSL',
        'USE_WEBRTC_DEV_BRANCH',
        'timezone=_timezone',
      ],
      'include_dirs': [
        './<(libjingle_source)',
        '../',
      ],
      'includes': ['libjingle_common.gypi', ],
      # TODO(sergeyu): Depend on webrtc/base.gyp:rtc_base_approved?
      'sources': [
        '<(webrtc_base)/asyncfile.cc',
        '<(webrtc_base)/asyncfile.h',
        '<(webrtc_base)/asynchttprequest.cc',
        '<(webrtc_base)/asynchttprequest.h',
        '<(webrtc_base)/asyncpacketsocket.cc',
        '<(webrtc_base)/asyncpacketsocket.h',
        '<(webrtc_base)/asyncresolverinterface.cc',
        '<(webrtc_base)/asyncresolverinterface.h',
        '<(webrtc_base)/asyncsocket.cc',
        '<(webrtc_base)/asyncsocket.h',
        '<(webrtc_base)/asynctcpsocket.cc',
        '<(webrtc_base)/asynctcpsocket.h',
        '<(webrtc_base)/asyncudpsocket.cc',
        '<(webrtc_base)/asyncudpsocket.h',
        '<(webrtc_base)/autodetectproxy.cc',
        '<(webrtc_base)/autodetectproxy.h',
        '<(webrtc_base)/base64.cc',
        '<(webrtc_base)/base64.h',
        '<(webrtc_base)/basicdefs.h',
        '<(webrtc_base)/buffer.cc',
        '<(webrtc_base)/buffer.h',
        '<(webrtc_base)/bytebuffer.cc',
        '<(webrtc_base)/bytebuffer.h',
        '<(webrtc_base)/byteorder.h',
        '<(webrtc_base)/checks.cc',
        '<(webrtc_base)/checks.h',
        '<(webrtc_base)/common.cc',
        '<(webrtc_base)/common.h',
        '<(webrtc_base)/crc32.cc',
        '<(webrtc_base)/crc32.h',
        '<(webrtc_base)/criticalsection.cc',
        '<(webrtc_base)/criticalsection.h',
        '<(webrtc_base)/cryptstring.cc',
        '<(webrtc_base)/cryptstring.h',
        '<(webrtc_base)/diskcache.cc',
        '<(webrtc_base)/diskcache.h',
        '<(webrtc_base)/dscp.h',
        '<(webrtc_base)/event.cc',
        '<(webrtc_base)/event.h',
        '<(webrtc_base)/event_tracer.cc',
        '<(webrtc_base)/event_tracer.h',
        '<(webrtc_base)/fileutils.cc',
        '<(webrtc_base)/fileutils.h',
        '<(webrtc_base)/firewallsocketserver.cc',
        '<(webrtc_base)/firewallsocketserver.h',
        '<(webrtc_base)/flags.cc',
        '<(webrtc_base)/flags.h',
        '<(webrtc_base)/helpers.cc',
        '<(webrtc_base)/helpers.h',
        '<(webrtc_base)/httpbase.cc',
        '<(webrtc_base)/httpbase.h',
        '<(webrtc_base)/httpclient.cc',
        '<(webrtc_base)/httpclient.h',
        '<(webrtc_base)/httpcommon-inl.h',
        '<(webrtc_base)/httpcommon.cc',
        '<(webrtc_base)/httpcommon.h',
        '<(webrtc_base)/httprequest.cc',
        '<(webrtc_base)/httprequest.h',
        '<(webrtc_base)/ipaddress.cc',
        '<(webrtc_base)/ipaddress.h',
        '<(webrtc_base)/linked_ptr.h',
        '<(webrtc_base)/logging.cc',
        '<(webrtc_base)/logging.h',
        '<(webrtc_base)/maccocoathreadhelper.h',
        '<(webrtc_base)/maccocoathreadhelper.mm',
        '<(webrtc_base)/macconversion.cc',
        '<(webrtc_base)/macconversion.h',
        '<(webrtc_base)/macutils.cc',
        '<(webrtc_base)/macutils.h',
        '<(webrtc_base)/md5.cc',
        '<(webrtc_base)/md5.h',
        '<(webrtc_base)/md5digest.h',
        '<(webrtc_base)/messagedigest.cc',
        '<(webrtc_base)/messagedigest.h',
        '<(webrtc_base)/messagehandler.cc',
        '<(webrtc_base)/messagehandler.h',
        '<(webrtc_base)/messagequeue.cc',
        '<(webrtc_base)/messagequeue.h',
        '<(webrtc_base)/nethelpers.cc',
        '<(webrtc_base)/nethelpers.h',
        '<(webrtc_base)/network.cc',
        '<(webrtc_base)/network.h',
        '<(webrtc_base)/networkmonitor.cc',
        '<(webrtc_base)/networkmonitor.h',
        '<(webrtc_base)/nullsocketserver.h',
        '<(webrtc_base)/openssladapter.cc',
        '<(webrtc_base)/openssldigest.cc',
        '<(webrtc_base)/opensslidentity.cc',
        '<(webrtc_base)/opensslstreamadapter.cc',
        '<(webrtc_base)/pathutils.cc',
        '<(webrtc_base)/pathutils.h',
        '<(webrtc_base)/physicalsocketserver.cc',
        '<(webrtc_base)/physicalsocketserver.h',
        '<(webrtc_base)/platform_thread.cc',
        '<(webrtc_base)/platform_thread.h',
        '<(webrtc_base)/proxydetect.cc',
        '<(webrtc_base)/proxydetect.h',
        '<(webrtc_base)/proxyinfo.cc',
        '<(webrtc_base)/proxyinfo.h',
        '<(webrtc_base)/ratelimiter.cc',
        '<(webrtc_base)/ratelimiter.h',
        '<(webrtc_base)/ratetracker.cc',
        '<(webrtc_base)/ratetracker.h',
        '<(webrtc_base)/scoped_autorelease_pool.h',
        '<(webrtc_base)/scoped_autorelease_pool.mm',
        '<(webrtc_base)/scoped_ptr.h',
        '<(webrtc_base)/sec_buffer.h',
        '<(webrtc_base)/sha1.cc',
        '<(webrtc_base)/sha1.h',
        '<(webrtc_base)/sha1digest.h',
        '<(webrtc_base)/signalthread.cc',
        '<(webrtc_base)/signalthread.h',
        '<(webrtc_base)/sigslot.cc',
        '<(webrtc_base)/sigslot.h',
        '<(webrtc_base)/sigslotrepeater.h',
        '<(webrtc_base)/socket.h',
        '<(webrtc_base)/socketadapters.cc',
        '<(webrtc_base)/socketadapters.h',
        '<(webrtc_base)/socketaddress.cc',
        '<(webrtc_base)/socketaddress.h',
        '<(webrtc_base)/socketaddresspair.cc',
        '<(webrtc_base)/socketaddresspair.h',
        '<(webrtc_base)/socketfactory.h',
        '<(webrtc_base)/socketpool.cc',
        '<(webrtc_base)/socketpool.h',
        '<(webrtc_base)/socketserver.h',
        '<(webrtc_base)/socketstream.cc',
        '<(webrtc_base)/socketstream.h',
        '<(webrtc_base)/ssladapter.cc',
        '<(webrtc_base)/ssladapter.h',
        '<(webrtc_base)/sslfingerprint.cc',
        '<(webrtc_base)/sslfingerprint.h',
        '<(webrtc_base)/sslidentity.cc',
        '<(webrtc_base)/sslidentity.h',
        '<(webrtc_base)/sslsocketfactory.cc',
        '<(webrtc_base)/sslsocketfactory.h',
        '<(webrtc_base)/sslstreamadapter.cc',
        '<(webrtc_base)/sslstreamadapter.h',
        '<(webrtc_base)/sslstreamadapterhelper.cc',
        '<(webrtc_base)/sslstreamadapterhelper.h',
        '<(webrtc_base)/stream.cc',
        '<(webrtc_base)/stream.h',
        '<(webrtc_base)/stringencode.cc',
        '<(webrtc_base)/stringencode.h',
        '<(webrtc_base)/stringutils.cc',
        '<(webrtc_base)/stringutils.h',
        '<(webrtc_base)/task.cc',
        '<(webrtc_base)/task.h',
        '<(webrtc_base)/taskparent.cc',
        '<(webrtc_base)/taskparent.h',
        '<(webrtc_base)/taskrunner.cc',
        '<(webrtc_base)/taskrunner.h',
        '<(webrtc_base)/template_util.h',
        '<(webrtc_base)/thread.cc',
        '<(webrtc_base)/thread.h',
        '<(webrtc_base)/timeutils.cc',
        '<(webrtc_base)/timeutils.h',
        '<(webrtc_base)/timing.cc',
        '<(webrtc_base)/timing.h',
        '<(webrtc_base)/unixfilesystem.cc',
        '<(webrtc_base)/unixfilesystem.h',
        '<(webrtc_base)/urlencode.cc',
        '<(webrtc_base)/urlencode.h',
        '<(webrtc_base)/win32.cc',
        '<(webrtc_base)/win32.h',
        '<(webrtc_base)/win32filesystem.cc',
        '<(webrtc_base)/win32filesystem.h',
        '<(webrtc_base)/win32securityerrors.cc',
        '<(webrtc_base)/win32window.cc',
        '<(webrtc_base)/win32window.h',
        '<(webrtc_base)/winfirewall.cc',
        '<(webrtc_base)/winfirewall.h',
        '<(webrtc_base)/winping.cc',
        '<(webrtc_base)/winping.h',
        '<(webrtc_base)/worker.cc',
        '<(webrtc_base)/worker.h',
        '<(webrtc_xmllite)/qname.cc',
        '<(webrtc_xmllite)/qname.h',
        '<(webrtc_xmllite)/xmlbuilder.cc',
        '<(webrtc_xmllite)/xmlbuilder.h',
        '<(webrtc_xmllite)/xmlconstants.cc',
        '<(webrtc_xmllite)/xmlconstants.h',
        '<(webrtc_xmllite)/xmlelement.cc',
        '<(webrtc_xmllite)/xmlelement.h',
        '<(webrtc_xmllite)/xmlnsstack.cc',
        '<(webrtc_xmllite)/xmlnsstack.h',
        '<(webrtc_xmllite)/xmlparser.cc',
        '<(webrtc_xmllite)/xmlparser.h',
        '<(webrtc_xmllite)/xmlprinter.cc',
        '<(webrtc_xmllite)/xmlprinter.h',
        '<(webrtc_system_wrappers)/include/field_trial_default.h',
        '<(webrtc_system_wrappers)/include/field_trial.h',
        '<(webrtc_system_wrappers)/source/field_trial_default.cc',
      ],
      'sources!': [
        # Compiled as part of libjingle_p2p_constants_nacl.
        '<(webrtc_p2p)/base/constants.cc',
        '<(webrtc_p2p)/base/constants.h',
        # For NACL, we have the field_trial_default and don't need the
        # field_trail.cc.
        'overrides/field_trial.cc',
      ],
      'sources/': [
        ['exclude', '/mac[a-z]+\\.(h|cc)$'],
        ['exclude', '/scoped_autorelease_pool\\.(h|mm)$'],
      ],
      'conditions': [
        ['OS!="win"', {
          'sources/': [
            ['exclude', '/win[a-z0-9]+\\.(h|cc)$'],
          ],
        }],
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          './overrides',
          './<(libjingle_source)',
          '../../third_party/webrtc_overrides',
          '../../third_party',
          '../../third_party/webrtc',
        ],
        'defines': [
          'EXPAT_RELATIVE_PATH',
          'FEATURE_ENABLE_SSL',
          'GTEST_RELATIVE_PATH',
          'NO_MAIN_THREAD_WRAPPING',
          'NO_SOUND_SYSTEM',
          'WEBRTC_POSIX',
          'SRTP_RELATIVE_PATH',
          'SSL_USE_OPENSSL',
          'USE_WEBRTC_DEV_BRANCH',
        ],
      },
      'export_dependent_settings': [
        '<(DEPTH)/native_client_sdk/native_client_sdk_untrusted.gyp:nacl_io_untrusted',
      ],
    },  # end of target 'libjingle_nacl'

    {
      'target_name': 'libjingle_p2p_constants_nacl',
      'type': 'none',
      'variables': {
        'nlib_target': 'libjingle_p2p_constants_nacl.a',
        'nacl_untrusted_build': 1,
        'build_glibc': 0,
        'build_newlib': 1,
        'build_pnacl_newlib': 1,
      },
      'include_dirs': [
        './<(libjingle_source)',
        '../'
      ],
      'sources': [
        '<(webrtc_p2p)/base/constants.cc',
        '<(webrtc_p2p)/base/constants.h',
      ],
    },  # end of target 'libjingle_p2p_constants_nacl'
  ],
}