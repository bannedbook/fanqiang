
import socket
from front_base.connect_creator import ConnectCreator as ConnectCreatorBase


class ConnectCreator(ConnectCreatorBase):
    def check_cert(self, ssl_sock):
        cert_chain = ssl_sock.get_peer_cert_chain()
        if not cert_chain:
            raise socket.error(' certificate is none, sni:%s' % ssl_sock.sni)

        self.get_ssl_cert_domain(ssl_sock)
        issuer_commonname = next((v for k, v in cert_chain[0].get_issuer().get_components() if k == 'CN'), '')
        if self.debug:
            for cert in cert_chain:
                for k, v in cert.get_issuer().get_components():
                    if k != "CN":
                        continue
                    cn = v
                    self.logger.debug("cn:%s", cn)

            self.logger.debug("issued by:%s", issuer_commonname)
            self.logger.debug("Common Name:%s", ssl_sock.domain)

        if self.config.check_commonname and not issuer_commonname.startswith(self.config.check_commonname):
            raise socket.error(' certficate is issued by %r' % (issuer_commonname))

        if not self.config.check_sni:
            return True

        cert = ssl_sock.get_peer_certificate()
        if not cert:
            raise socket.error('certficate is none')

        # get_subj_alt_name cost near 100ms. be careful.
        try:
            alt_names = ConnectCreator.get_subj_alt_name(cert)
        except Exception as e:
            # self.logger.warn("get_subj_alt_name fail:%r", e)
            alt_names = [""]

        if self.debug:
            self.logger.debug('alt names: "%s"', '", "'.join(alt_names))

        if 'cloudfront.net' in alt_names:
            return True

        alt_names = tuple(alt_names)
        if ssl_sock.sni.endswith(alt_names):
            return True

        raise socket.error('check sni:%s fail, alt_names:%s' % (ssl_sock.sni, alt_names))
