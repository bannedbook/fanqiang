#
# This file is part of pyasn1 software.
#
# Copyright (c) 2005-2018, Ilya Etingof <etingof@gmail.com>
# License: http://snmplabs.com/pyasn1/license.html
#
from pyasn1 import error
from pyasn1.codec.ber import encoder
from pyasn1.compat.octets import str2octs, null
from pyasn1.type import univ
from pyasn1.type import useful

__all__ = ['encode']


class BooleanEncoder(encoder.IntegerEncoder):
    def encodeValue(self, value, asn1Spec, encodeFun, **options):
        if value == 0:
            substrate = (0,)
        else:
            substrate = (255,)
        return substrate, False, False


class RealEncoder(encoder.RealEncoder):
    def _chooseEncBase(self, value):
        m, b, e = value
        return self._dropFloatingPoint(m, b, e)


# specialized GeneralStringEncoder here

class TimeEncoderMixIn(object):
    zchar, = str2octs('Z')
    pluschar, = str2octs('+')
    minuschar, = str2octs('-')
    commachar, = str2octs(',')
    minLength = 12
    maxLength = 19

    def encodeValue(self, value, asn1Spec, encodeFun, **options):
        # Encoding constraints:
        # - minutes are mandatory, seconds are optional
        # - subseconds must NOT be zero
        # - no hanging fraction dot
        # - time in UTC (Z)
        # - only dot is allowed for fractions

        if asn1Spec is not None:
            value = asn1Spec.clone(value)

        octets = value.asOctets()

        if not self.minLength < len(octets) < self.maxLength:
            raise error.PyAsn1Error('Length constraint violated: %r' % value)

        if self.pluschar in octets or self.minuschar in octets:
            raise error.PyAsn1Error('Must be UTC time: %r' % octets)

        if octets[-1] != self.zchar:
            raise error.PyAsn1Error('Missing "Z" time zone specifier: %r' % octets)

        if self.commachar in octets:
            raise error.PyAsn1Error('Comma in fractions disallowed: %r' % value)

        options.update(maxChunkSize=1000)

        return encoder.OctetStringEncoder.encodeValue(
            self, value, asn1Spec, encodeFun, **options
        )


class GeneralizedTimeEncoder(TimeEncoderMixIn, encoder.OctetStringEncoder):
    minLength = 12
    maxLength = 19


class UTCTimeEncoder(TimeEncoderMixIn, encoder.OctetStringEncoder):
    minLength = 10
    maxLength = 14


class SetEncoder(encoder.SequenceEncoder):
    @staticmethod
    def _componentSortKey(componentAndType):
        """Sort SET components by tag

        Sort regardless of the Choice value (static sort)
        """
        component, asn1Spec = componentAndType

        if asn1Spec is None:
            asn1Spec = component

        if asn1Spec.typeId == univ.Choice.typeId and not asn1Spec.tagSet:
            if asn1Spec.tagSet:
                return asn1Spec.tagSet
            else:
                return asn1Spec.componentType.minTagSet
        else:
            return asn1Spec.tagSet

    def encodeValue(self, value, asn1Spec, encodeFun, **options):

        substrate = null

        comps = []
        compsMap = {}

        if asn1Spec is None:
            # instance of ASN.1 schema
            value.verifySizeSpec()

            namedTypes = value.componentType

            for idx, component in enumerate(value.values()):
                if namedTypes:
                    namedType = namedTypes[idx]

                    if namedType.isOptional and not component.isValue:
                            continue

                    if namedType.isDefaulted and component == namedType.asn1Object:
                            continue

                    compsMap[id(component)] = namedType

                else:
                    compsMap[id(component)] = None

                comps.append((component, asn1Spec))

        else:
            # bare Python value + ASN.1 schema
            for idx, namedType in enumerate(asn1Spec.componentType.namedTypes):

                try:
                    component = value[namedType.name]

                except KeyError:
                    raise error.PyAsn1Error('Component name "%s" not found in %r' % (namedType.name, value))

                if namedType.isOptional and namedType.name not in value:
                    continue

                if namedType.isDefaulted and component == namedType.asn1Object:
                    continue

                compsMap[id(component)] = namedType
                comps.append((component, asn1Spec[idx]))

        for comp, compType in sorted(comps, key=self._componentSortKey):
            namedType = compsMap[id(comp)]

            if namedType:
                options.update(ifNotEmpty=namedType.isOptional)

            chunk = encodeFun(comp, compType, **options)

            # wrap open type blob if needed
            if namedType and namedType.openType:
                wrapType = namedType.asn1Object
                if wrapType.tagSet and not wrapType.isSameTypeWith(comp):
                    chunk = encodeFun(chunk, wrapType, **options)

            substrate += chunk

        return substrate, True, True


class SetOfEncoder(encoder.SequenceOfEncoder):
    def encodeValue(self, value, asn1Spec, encodeFun, **options):
        if asn1Spec is None:
            value.verifySizeSpec()
        else:
            asn1Spec = asn1Spec.componentType

        components = [encodeFun(x, asn1Spec, **options)
                      for x in value]

        # sort by serialised and padded components
        if len(components) > 1:
            zero = str2octs('\x00')
            maxLen = max(map(len, components))
            paddedComponents = [
                (x.ljust(maxLen, zero), x) for x in components
                ]
            paddedComponents.sort(key=lambda x: x[0])

            components = [x[1] for x in paddedComponents]

        substrate = null.join(components)

        return substrate, True, True


class SequenceEncoder(encoder.SequenceEncoder):
    omitEmptyOptionals = True


class SequenceOfEncoder(encoder.SequenceOfEncoder):
    def encodeValue(self, value, asn1Spec, encodeFun, **options):

        if options.get('ifNotEmpty', False) and not len(value):
            return null, True, True

        if asn1Spec is None:
            value.verifySizeSpec()
        else:
            asn1Spec = asn1Spec.componentType

        substrate = null

        for idx, component in enumerate(value):
            substrate += encodeFun(value[idx], asn1Spec, **options)

        return substrate, True, True


tagMap = encoder.tagMap.copy()
tagMap.update({
    univ.Boolean.tagSet: BooleanEncoder(),
    univ.Real.tagSet: RealEncoder(),
    useful.GeneralizedTime.tagSet: GeneralizedTimeEncoder(),
    useful.UTCTime.tagSet: UTCTimeEncoder(),
    # Sequence & Set have same tags as SequenceOf & SetOf
    univ.SetOf.tagSet: SetOfEncoder(),
    univ.Sequence.typeId: SequenceEncoder()
})

typeMap = encoder.typeMap.copy()
typeMap.update({
    univ.Boolean.typeId: BooleanEncoder(),
    univ.Real.typeId: RealEncoder(),
    useful.GeneralizedTime.typeId: GeneralizedTimeEncoder(),
    useful.UTCTime.typeId: UTCTimeEncoder(),
    # Sequence & Set have same tags as SequenceOf & SetOf
    univ.Set.typeId: SetEncoder(),
    univ.SetOf.typeId: SetOfEncoder(),
    univ.Sequence.typeId: SequenceEncoder(),
    univ.SequenceOf.typeId: SequenceOfEncoder()
})


class Encoder(encoder.Encoder):
    fixedDefLengthMode = False
    fixedChunkSize = 1000

#: Turns ASN.1 object into CER octet stream.
#:
#: Takes any ASN.1 object (e.g. :py:class:`~pyasn1.type.base.PyAsn1Item` derivative)
#: walks all its components recursively and produces a CER octet stream.
#:
#: Parameters
#: ----------
#: value: either a Python or pyasn1 object (e.g. :py:class:`~pyasn1.type.base.PyAsn1Item` derivative)
#:     A Python or pyasn1 object to encode. If Python object is given, `asnSpec`
#:     parameter is required to guide the encoding process.
#:
#: Keyword Args
#: ------------
#: asn1Spec:
#:     Optional ASN.1 schema or value object e.g. :py:class:`~pyasn1.type.base.PyAsn1Item` derivative
#:
#: Returns
#: -------
#: : :py:class:`bytes` (Python 3) or :py:class:`str` (Python 2)
#:     Given ASN.1 object encoded into BER octet-stream
#:
#: Raises
#: ------
#: :py:class:`~pyasn1.error.PyAsn1Error`
#:     On encoding errors
#:
#: Examples
#: --------
#: Encode Python value into CER with ASN.1 schema
#:
#: .. code-block:: pycon
#:
#:    >>> seq = SequenceOf(componentType=Integer())
#:    >>> encode([1, 2, 3], asn1Spec=seq)
#:    b'0\x80\x02\x01\x01\x02\x01\x02\x02\x01\x03\x00\x00'
#:
#: Encode ASN.1 value object into CER
#:
#: .. code-block:: pycon
#:
#:    >>> seq = SequenceOf(componentType=Integer())
#:    >>> seq.extend([1, 2, 3])
#:    >>> encode(seq)
#:    b'0\x80\x02\x01\x01\x02\x01\x02\x02\x01\x03\x00\x00'
#:
encode = Encoder(tagMap, typeMap)

# EncoderFactory queries class instance and builds a map of tags -> encoders
