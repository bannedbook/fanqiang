import enum
import sys
import unittest
from enum import Enum, IntEnum, unique, EnumMeta
from pickle import dumps, loads, PicklingError, HIGHEST_PROTOCOL

pyver = float('%s.%s' % sys.version_info[:2])

try:
    any
except NameError:
    def any(iterable):
        for element in iterable:
            if element:
                return True
        return False

try:
    unicode
except NameError:
    unicode = str

try:
    from collections import OrderedDict
except ImportError:
    OrderedDict = None

# for pickle tests
try:
    class Stooges(Enum):
        LARRY = 1
        CURLY = 2
        MOE = 3
except Exception:
    Stooges = sys.exc_info()[1]

try:
    class IntStooges(int, Enum):
        LARRY = 1
        CURLY = 2
        MOE = 3
except Exception:
    IntStooges = sys.exc_info()[1]

try:
    class FloatStooges(float, Enum):
        LARRY = 1.39
        CURLY = 2.72
        MOE = 3.142596
except Exception:
    FloatStooges = sys.exc_info()[1]

# for pickle test and subclass tests
try:
    class StrEnum(str, Enum):
        'accepts only string values'
    class Name(StrEnum):
        BDFL = 'Guido van Rossum'
        FLUFL = 'Barry Warsaw'
except Exception:
    Name = sys.exc_info()[1]

try:
    Question = Enum('Question', 'who what when where why', module=__name__)
except Exception:
    Question = sys.exc_info()[1]

try:
    Answer = Enum('Answer', 'him this then there because')
except Exception:
    Answer = sys.exc_info()[1]

try:
    Theory = Enum('Theory', 'rule law supposition', qualname='spanish_inquisition')
except Exception:
    Theory = sys.exc_info()[1]

# for doctests
try:
    class Fruit(Enum):
        tomato = 1
        banana = 2
        cherry = 3
except Exception:
    pass

def test_pickle_dump_load(assertion, source, target=None,
        protocol=(0, HIGHEST_PROTOCOL)):
    start, stop = protocol
    failures = []
    for protocol in range(start, stop+1):
        try:
            if target is None:
                assertion(loads(dumps(source, protocol=protocol)) is source)
            else:
                assertion(loads(dumps(source, protocol=protocol)), target)
        except Exception:
            exc, tb = sys.exc_info()[1:]
            failures.append('%2d: %s' %(protocol, exc))
    if failures:
        raise ValueError('Failed with protocols: %s' % ', '.join(failures))

def test_pickle_exception(assertion, exception, obj,
        protocol=(0, HIGHEST_PROTOCOL)):
    start, stop = protocol
    failures = []
    for protocol in range(start, stop+1):
        try:
            assertion(exception, dumps, obj, protocol=protocol)
        except Exception:
            exc = sys.exc_info()[1]
            failures.append('%d: %s %s' % (protocol, exc.__class__.__name__, exc))
    if failures:
        raise ValueError('Failed with protocols: %s' % ', '.join(failures))


class TestHelpers(unittest.TestCase):
    # _is_descriptor, _is_sunder, _is_dunder

    def test_is_descriptor(self):
        class foo:
            pass
        for attr in ('__get__','__set__','__delete__'):
            obj = foo()
            self.assertFalse(enum._is_descriptor(obj))
            setattr(obj, attr, 1)
            self.assertTrue(enum._is_descriptor(obj))

    def test_is_sunder(self):
        for s in ('_a_', '_aa_'):
            self.assertTrue(enum._is_sunder(s))

        for s in ('a', 'a_', '_a', '__a', 'a__', '__a__', '_a__', '__a_', '_',
                '__', '___', '____', '_____',):
            self.assertFalse(enum._is_sunder(s))

    def test_is_dunder(self):
        for s in ('__a__', '__aa__'):
            self.assertTrue(enum._is_dunder(s))
        for s in ('a', 'a_', '_a', '__a', 'a__', '_a_', '_a__', '__a_', '_',
                '__', '___', '____', '_____',):
            self.assertFalse(enum._is_dunder(s))


class TestEnum(unittest.TestCase):
    def setUp(self):
        class Season(Enum):
            SPRING = 1
            SUMMER = 2
            AUTUMN = 3
            WINTER = 4
        self.Season = Season

        class Konstants(float, Enum):
            E = 2.7182818
            PI = 3.1415926
            TAU = 2 * PI
        self.Konstants = Konstants

        class Grades(IntEnum):
            A = 5
            B = 4
            C = 3
            D = 2
            F = 0
        self.Grades = Grades

        class Directional(str, Enum):
            EAST = 'east'
            WEST = 'west'
            NORTH = 'north'
            SOUTH = 'south'
        self.Directional = Directional

        from datetime import date
        class Holiday(date, Enum):
            NEW_YEAR = 2013, 1, 1
            IDES_OF_MARCH = 2013, 3, 15
        self.Holiday = Holiday

    if pyver >= 3.0:     # do not specify custom `dir` on previous versions
        def test_dir_on_class(self):
            Season = self.Season
            self.assertEqual(
                set(dir(Season)),
                set(['__class__', '__doc__', '__members__', '__module__',
                    'SPRING', 'SUMMER', 'AUTUMN', 'WINTER']),
                )

        def test_dir_on_item(self):
            Season = self.Season
            self.assertEqual(
                set(dir(Season.WINTER)),
                set(['__class__', '__doc__', '__module__', 'name', 'value']),
                )

        def test_dir_with_added_behavior(self):
            class Test(Enum):
                this = 'that'
                these = 'those'
                def wowser(self):
                    return ("Wowser! I'm %s!" % self.name)
            self.assertEqual(
                    set(dir(Test)),
                    set(['__class__', '__doc__', '__members__', '__module__', 'this', 'these']),
                    )
            self.assertEqual(
                    set(dir(Test.this)),
                    set(['__class__', '__doc__', '__module__', 'name', 'value', 'wowser']),
                    )

        def test_dir_on_sub_with_behavior_on_super(self):
            # see issue22506
            class SuperEnum(Enum):
                def invisible(self):
                    return "did you see me?"
            class SubEnum(SuperEnum):
                sample = 5
            self.assertEqual(
                    set(dir(SubEnum.sample)),
                    set(['__class__', '__doc__', '__module__', 'name', 'value', 'invisible']),
                    )

    if pyver >= 2.7:    # OrderedDict first available here
        def test_members_is_ordereddict_if_ordered(self):
            class Ordered(Enum):
                __order__ = 'first second third'
                first = 'bippity'
                second = 'boppity'
                third = 'boo'
            self.assertTrue(type(Ordered.__members__) is OrderedDict)

        def test_members_is_ordereddict_if_not_ordered(self):
            class Unordered(Enum):
                this = 'that'
                these = 'those'
            self.assertTrue(type(Unordered.__members__) is OrderedDict)

    if pyver >= 3.0:     # all objects are ordered in Python 2.x
        def test_members_is_always_ordered(self):
            class AlwaysOrdered(Enum):
                first = 1
                second = 2
                third = 3
            self.assertTrue(type(AlwaysOrdered.__members__) is OrderedDict)

        def test_comparisons(self):
            def bad_compare():
                Season.SPRING > 4
            Season = self.Season
            self.assertNotEqual(Season.SPRING, 1)
            self.assertRaises(TypeError, bad_compare)

            class Part(Enum):
                SPRING = 1
                CLIP = 2
                BARREL = 3

            self.assertNotEqual(Season.SPRING, Part.SPRING)
            def bad_compare():
                Season.SPRING < Part.CLIP
            self.assertRaises(TypeError, bad_compare)

    def test_enum_in_enum_out(self):
        Season = self.Season
        self.assertTrue(Season(Season.WINTER) is Season.WINTER)

    def test_enum_value(self):
        Season = self.Season
        self.assertEqual(Season.SPRING.value, 1)

    def test_intenum_value(self):
        self.assertEqual(IntStooges.CURLY.value, 2)

    def test_enum(self):
        Season = self.Season
        lst = list(Season)
        self.assertEqual(len(lst), len(Season))
        self.assertEqual(len(Season), 4, Season)
        self.assertEqual(
            [Season.SPRING, Season.SUMMER, Season.AUTUMN, Season.WINTER], lst)

        for i, season in enumerate('SPRING SUMMER AUTUMN WINTER'.split()):
            i += 1
            e = Season(i)
            self.assertEqual(e, getattr(Season, season))
            self.assertEqual(e.value, i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, season)
            self.assertTrue(e in Season)
            self.assertTrue(type(e) is Season)
            self.assertTrue(isinstance(e, Season))
            self.assertEqual(str(e), 'Season.' + season)
            self.assertEqual(
                    repr(e),
                    '<Season.%s: %s>' % (season, i),
                    )

    def test_value_name(self):
        Season = self.Season
        self.assertEqual(Season.SPRING.name, 'SPRING')
        self.assertEqual(Season.SPRING.value, 1)
        def set_name(obj, new_value):
            obj.name = new_value
        def set_value(obj, new_value):
            obj.value = new_value
        self.assertRaises(AttributeError, set_name, Season.SPRING, 'invierno', )
        self.assertRaises(AttributeError, set_value, Season.SPRING, 2)

    def test_attribute_deletion(self):
        class Season(Enum):
            SPRING = 1
            SUMMER = 2
            AUTUMN = 3
            WINTER = 4

            def spam(cls):
                pass

        self.assertTrue(hasattr(Season, 'spam'))
        del Season.spam
        self.assertFalse(hasattr(Season, 'spam'))

        self.assertRaises(AttributeError, delattr, Season, 'SPRING')
        self.assertRaises(AttributeError, delattr, Season, 'DRY')
        self.assertRaises(AttributeError, delattr, Season.SPRING, 'name')

    def test_invalid_names(self):
        def create_bad_class_1():
            class Wrong(Enum):
                mro = 9
        def create_bad_class_2():
            class Wrong(Enum):
                _reserved_ = 3
        self.assertRaises(ValueError, create_bad_class_1)
        self.assertRaises(ValueError, create_bad_class_2)

    def test_contains(self):
        Season = self.Season
        self.assertTrue(Season.AUTUMN in Season)
        self.assertTrue(3 not in Season)

        val = Season(3)
        self.assertTrue(val in Season)

        class OtherEnum(Enum):
            one = 1; two = 2
        self.assertTrue(OtherEnum.two not in Season)

    if pyver >= 2.6:     # when `format` came into being

        def test_format_enum(self):
            Season = self.Season
            self.assertEqual('{0}'.format(Season.SPRING),
                             '{0}'.format(str(Season.SPRING)))
            self.assertEqual( '{0:}'.format(Season.SPRING),
                              '{0:}'.format(str(Season.SPRING)))
            self.assertEqual('{0:20}'.format(Season.SPRING),
                             '{0:20}'.format(str(Season.SPRING)))
            self.assertEqual('{0:^20}'.format(Season.SPRING),
                             '{0:^20}'.format(str(Season.SPRING)))
            self.assertEqual('{0:>20}'.format(Season.SPRING),
                             '{0:>20}'.format(str(Season.SPRING)))
            self.assertEqual('{0:<20}'.format(Season.SPRING),
                             '{0:<20}'.format(str(Season.SPRING)))

        def test_format_enum_custom(self):
            class TestFloat(float, Enum):
                one = 1.0
                two = 2.0
                def __format__(self, spec):
                    return 'TestFloat success!'
            self.assertEqual('{0}'.format(TestFloat.one), 'TestFloat success!')

        def assertFormatIsValue(self, spec, member):
            self.assertEqual(spec.format(member), spec.format(member.value))

        def test_format_enum_date(self):
            Holiday = self.Holiday
            self.assertFormatIsValue('{0}', Holiday.IDES_OF_MARCH)
            self.assertFormatIsValue('{0:}', Holiday.IDES_OF_MARCH)
            self.assertFormatIsValue('{0:20}', Holiday.IDES_OF_MARCH)
            self.assertFormatIsValue('{0:^20}', Holiday.IDES_OF_MARCH)
            self.assertFormatIsValue('{0:>20}', Holiday.IDES_OF_MARCH)
            self.assertFormatIsValue('{0:<20}', Holiday.IDES_OF_MARCH)
            self.assertFormatIsValue('{0:%Y %m}', Holiday.IDES_OF_MARCH)
            self.assertFormatIsValue('{0:%Y %m %M:00}', Holiday.IDES_OF_MARCH)

        def test_format_enum_float(self):
            Konstants = self.Konstants
            self.assertFormatIsValue('{0}', Konstants.TAU)
            self.assertFormatIsValue('{0:}', Konstants.TAU)
            self.assertFormatIsValue('{0:20}', Konstants.TAU)
            self.assertFormatIsValue('{0:^20}', Konstants.TAU)
            self.assertFormatIsValue('{0:>20}', Konstants.TAU)
            self.assertFormatIsValue('{0:<20}', Konstants.TAU)
            self.assertFormatIsValue('{0:n}', Konstants.TAU)
            self.assertFormatIsValue('{0:5.2}', Konstants.TAU)
            self.assertFormatIsValue('{0:f}', Konstants.TAU)

        def test_format_enum_int(self):
            Grades = self.Grades
            self.assertFormatIsValue('{0}', Grades.C)
            self.assertFormatIsValue('{0:}', Grades.C)
            self.assertFormatIsValue('{0:20}', Grades.C)
            self.assertFormatIsValue('{0:^20}', Grades.C)
            self.assertFormatIsValue('{0:>20}', Grades.C)
            self.assertFormatIsValue('{0:<20}', Grades.C)
            self.assertFormatIsValue('{0:+}', Grades.C)
            self.assertFormatIsValue('{0:08X}', Grades.C)
            self.assertFormatIsValue('{0:b}', Grades.C)

        def test_format_enum_str(self):
            Directional = self.Directional
            self.assertFormatIsValue('{0}', Directional.WEST)
            self.assertFormatIsValue('{0:}', Directional.WEST)
            self.assertFormatIsValue('{0:20}', Directional.WEST)
            self.assertFormatIsValue('{0:^20}', Directional.WEST)
            self.assertFormatIsValue('{0:>20}', Directional.WEST)
            self.assertFormatIsValue('{0:<20}', Directional.WEST)

    def test_hash(self):
        Season = self.Season
        dates = {}
        dates[Season.WINTER] = '1225'
        dates[Season.SPRING] = '0315'
        dates[Season.SUMMER] = '0704'
        dates[Season.AUTUMN] = '1031'
        self.assertEqual(dates[Season.AUTUMN], '1031')

    def test_enum_duplicates(self):
        __order__ = "SPRING SUMMER AUTUMN WINTER"
        class Season(Enum):
            SPRING = 1
            SUMMER = 2
            AUTUMN = FALL = 3
            WINTER = 4
            ANOTHER_SPRING = 1
        lst = list(Season)
        self.assertEqual(
            lst,
            [Season.SPRING, Season.SUMMER,
             Season.AUTUMN, Season.WINTER,
            ])
        self.assertTrue(Season.FALL is Season.AUTUMN)
        self.assertEqual(Season.FALL.value, 3)
        self.assertEqual(Season.AUTUMN.value, 3)
        self.assertTrue(Season(3) is Season.AUTUMN)
        self.assertTrue(Season(1) is Season.SPRING)
        self.assertEqual(Season.FALL.name, 'AUTUMN')
        self.assertEqual(
                set([k for k,v in Season.__members__.items() if v.name != k]),
                set(['FALL', 'ANOTHER_SPRING']),
                )

    if pyver >= 3.0:
        cls = vars()
        result = {'Enum':Enum}
        exec("""def test_duplicate_name(self):
            with self.assertRaises(TypeError):
                class Color(Enum):
                    red = 1
                    green = 2
                    blue = 3
                    red = 4

            with self.assertRaises(TypeError):
                class Color(Enum):
                    red = 1
                    green = 2
                    blue = 3
                    def red(self):
                        return 'red'

            with self.assertRaises(TypeError):
                class Color(Enum):
                    @property

                    def red(self):
                        return 'redder'
                    red = 1
                    green = 2
                    blue = 3""",
            result)
        cls['test_duplicate_name'] = result['test_duplicate_name']

    def test_enum_with_value_name(self):
        class Huh(Enum):
            name = 1
            value = 2
        self.assertEqual(
            list(Huh),
            [Huh.name, Huh.value],
            )
        self.assertTrue(type(Huh.name) is Huh)
        self.assertEqual(Huh.name.name, 'name')
        self.assertEqual(Huh.name.value, 1)

    def test_intenum_from_scratch(self):
        class phy(int, Enum):
            pi = 3
            tau = 2 * pi
        self.assertTrue(phy.pi < phy.tau)

    def test_intenum_inherited(self):
        class IntEnum(int, Enum):
            pass
        class phy(IntEnum):
            pi = 3
            tau = 2 * pi
        self.assertTrue(phy.pi < phy.tau)

    def test_floatenum_from_scratch(self):
        class phy(float, Enum):
            pi = 3.1415926
            tau = 2 * pi
        self.assertTrue(phy.pi < phy.tau)

    def test_floatenum_inherited(self):
        class FloatEnum(float, Enum):
            pass
        class phy(FloatEnum):
            pi = 3.1415926
            tau = 2 * pi
        self.assertTrue(phy.pi < phy.tau)

    def test_strenum_from_scratch(self):
        class phy(str, Enum):
            pi = 'Pi'
            tau = 'Tau'
        self.assertTrue(phy.pi < phy.tau)

    def test_strenum_inherited(self):
        class StrEnum(str, Enum):
            pass
        class phy(StrEnum):
            pi = 'Pi'
            tau = 'Tau'
        self.assertTrue(phy.pi < phy.tau)

    def test_intenum(self):
        class WeekDay(IntEnum):
            SUNDAY = 1
            MONDAY = 2
            TUESDAY = 3
            WEDNESDAY = 4
            THURSDAY = 5
            FRIDAY = 6
            SATURDAY = 7

        self.assertEqual(['a', 'b', 'c'][WeekDay.MONDAY], 'c')
        self.assertEqual([i for i in range(WeekDay.TUESDAY)], [0, 1, 2])

        lst = list(WeekDay)
        self.assertEqual(len(lst), len(WeekDay))
        self.assertEqual(len(WeekDay), 7)
        target = 'SUNDAY MONDAY TUESDAY WEDNESDAY THURSDAY FRIDAY SATURDAY'
        target = target.split()
        for i, weekday in enumerate(target):
            i += 1
            e = WeekDay(i)
            self.assertEqual(e, i)
            self.assertEqual(int(e), i)
            self.assertEqual(e.name, weekday)
            self.assertTrue(e in WeekDay)
            self.assertEqual(lst.index(e)+1, i)
            self.assertTrue(0 < e < 8)
            self.assertTrue(type(e) is WeekDay)
            self.assertTrue(isinstance(e, int))
            self.assertTrue(isinstance(e, Enum))

    def test_intenum_duplicates(self):
        class WeekDay(IntEnum):
            __order__ = 'SUNDAY MONDAY TUESDAY WEDNESDAY THURSDAY FRIDAY SATURDAY'
            SUNDAY = 1
            MONDAY = 2
            TUESDAY = TEUSDAY = 3
            WEDNESDAY = 4
            THURSDAY = 5
            FRIDAY = 6
            SATURDAY = 7
        self.assertTrue(WeekDay.TEUSDAY is WeekDay.TUESDAY)
        self.assertEqual(WeekDay(3).name, 'TUESDAY')
        self.assertEqual([k for k,v in WeekDay.__members__.items()
                if v.name != k], ['TEUSDAY', ])

    def test_pickle_enum(self):
        if isinstance(Stooges, Exception):
            raise Stooges
        test_pickle_dump_load(self.assertTrue, Stooges.CURLY)
        test_pickle_dump_load(self.assertTrue, Stooges)

    def test_pickle_int(self):
        if isinstance(IntStooges, Exception):
            raise IntStooges
        test_pickle_dump_load(self.assertTrue, IntStooges.CURLY)
        test_pickle_dump_load(self.assertTrue, IntStooges)

    def test_pickle_float(self):
        if isinstance(FloatStooges, Exception):
            raise FloatStooges
        test_pickle_dump_load(self.assertTrue, FloatStooges.CURLY)
        test_pickle_dump_load(self.assertTrue, FloatStooges)

    def test_pickle_enum_function(self):
        if isinstance(Answer, Exception):
            raise Answer
        test_pickle_dump_load(self.assertTrue, Answer.him)
        test_pickle_dump_load(self.assertTrue, Answer)

    def test_pickle_enum_function_with_module(self):
        if isinstance(Question, Exception):
            raise Question
        test_pickle_dump_load(self.assertTrue, Question.who)
        test_pickle_dump_load(self.assertTrue, Question)

    if pyver == 3.4:
        def test_class_nested_enum_and_pickle_protocol_four(self):
            # would normally just have this directly in the class namespace
            class NestedEnum(Enum):
                twigs = 'common'
                shiny = 'rare'

            self.__class__.NestedEnum = NestedEnum
            self.NestedEnum.__qualname__ = '%s.NestedEnum' % self.__class__.__name__
            test_pickle_exception(
                    self.assertRaises, PicklingError, self.NestedEnum.twigs,
                    protocol=(0, 3))
            test_pickle_dump_load(self.assertTrue, self.NestedEnum.twigs,
                    protocol=(4, HIGHEST_PROTOCOL))

    elif pyver == 3.5:
        def test_class_nested_enum_and_pickle_protocol_four(self):
            # would normally just have this directly in the class namespace
            class NestedEnum(Enum):
                twigs = 'common'
                shiny = 'rare'

            self.__class__.NestedEnum = NestedEnum
            self.NestedEnum.__qualname__ = '%s.NestedEnum' % self.__class__.__name__
            test_pickle_dump_load(self.assertTrue, self.NestedEnum.twigs,
                    protocol=(0, HIGHEST_PROTOCOL))

    def test_exploding_pickle(self):
        BadPickle = Enum('BadPickle', 'dill sweet bread-n-butter')
        enum._make_class_unpicklable(BadPickle)
        globals()['BadPickle'] = BadPickle
        test_pickle_exception(self.assertRaises, TypeError, BadPickle.dill)
        test_pickle_exception(self.assertRaises, PicklingError, BadPickle)

    def test_string_enum(self):
        class SkillLevel(str, Enum):
            master = 'what is the sound of one hand clapping?'
            journeyman = 'why did the chicken cross the road?'
            apprentice = 'knock, knock!'
        self.assertEqual(SkillLevel.apprentice, 'knock, knock!')

    def test_getattr_getitem(self):
        class Period(Enum):
            morning = 1
            noon = 2
            evening = 3
            night = 4
        self.assertTrue(Period(2) is Period.noon)
        self.assertTrue(getattr(Period, 'night') is Period.night)
        self.assertTrue(Period['morning'] is Period.morning)

    def test_getattr_dunder(self):
        Season = self.Season
        self.assertTrue(getattr(Season, '__hash__'))

    def test_iteration_order(self):
        class Season(Enum):
            __order__ = 'SUMMER WINTER AUTUMN SPRING'
            SUMMER = 2
            WINTER = 4
            AUTUMN = 3
            SPRING = 1
        self.assertEqual(
                list(Season),
                [Season.SUMMER, Season.WINTER, Season.AUTUMN, Season.SPRING],
                )

    def test_iteration_order_reversed(self):
        self.assertEqual(
                list(reversed(self.Season)),
                [self.Season.WINTER, self.Season.AUTUMN, self.Season.SUMMER,
                 self.Season.SPRING]
                )

    def test_iteration_order_with_unorderable_values(self):
        class Complex(Enum):
            a = complex(7, 9)
            b = complex(3.14, 2)
            c = complex(1, -1)
            d = complex(-77, 32)
        self.assertEqual(
                list(Complex),
                [Complex.a, Complex.b, Complex.c, Complex.d],
                )

    def test_programatic_function_string(self):
        SummerMonth = Enum('SummerMonth', 'june july august')
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split()):
            i += 1
            e = SummerMonth(i)
            self.assertEqual(int(e.value), i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertTrue(e in SummerMonth)
            self.assertTrue(type(e) is SummerMonth)

    def test_programatic_function_string_with_start(self):
        SummerMonth = Enum('SummerMonth', 'june july august', start=10)
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split(), 10):
            e = SummerMonth(i)
            self.assertEqual(int(e.value), i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertTrue(e in SummerMonth)
            self.assertTrue(type(e) is SummerMonth)

    def test_programatic_function_string_list(self):
        SummerMonth = Enum('SummerMonth', ['june', 'july', 'august'])
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split()):
            i += 1
            e = SummerMonth(i)
            self.assertEqual(int(e.value), i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertTrue(e in SummerMonth)
            self.assertTrue(type(e) is SummerMonth)

    def test_programatic_function_string_list_with_start(self):
        SummerMonth = Enum('SummerMonth', ['june', 'july', 'august'], start=20)
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split(), 20):
            e = SummerMonth(i)
            self.assertEqual(int(e.value), i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertTrue(e in SummerMonth)
            self.assertTrue(type(e) is SummerMonth)

    def test_programatic_function_iterable(self):
        SummerMonth = Enum(
                'SummerMonth',
                (('june', 1), ('july', 2), ('august', 3))
                )
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split()):
            i += 1
            e = SummerMonth(i)
            self.assertEqual(int(e.value), i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertTrue(e in SummerMonth)
            self.assertTrue(type(e) is SummerMonth)

    def test_programatic_function_from_dict(self):
        SummerMonth = Enum(
                'SummerMonth',
                dict((('june', 1), ('july', 2), ('august', 3)))
                )
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        if pyver < 3.0:
            self.assertEqual(
                    [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                    lst,
                    )
        for i, month in enumerate('june july august'.split()):
            i += 1
            e = SummerMonth(i)
            self.assertEqual(int(e.value), i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertTrue(e in SummerMonth)
            self.assertTrue(type(e) is SummerMonth)

    def test_programatic_function_type(self):
        SummerMonth = Enum('SummerMonth', 'june july august', type=int)
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split()):
            i += 1
            e = SummerMonth(i)
            self.assertEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertTrue(e in SummerMonth)
            self.assertTrue(type(e) is SummerMonth)

    def test_programatic_function_type_with_start(self):
        SummerMonth = Enum('SummerMonth', 'june july august', type=int, start=30)
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split(), 30):
            e = SummerMonth(i)
            self.assertEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertTrue(e in SummerMonth)
            self.assertTrue(type(e) is SummerMonth)

    def test_programatic_function_type_from_subclass(self):
        SummerMonth = IntEnum('SummerMonth', 'june july august')
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split()):
            i += 1
            e = SummerMonth(i)
            self.assertEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertTrue(e in SummerMonth)
            self.assertTrue(type(e) is SummerMonth)

    def test_programatic_function_type_from_subclass_with_start(self):
        SummerMonth = IntEnum('SummerMonth', 'june july august', start=40)
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate('june july august'.split(), 40):
            e = SummerMonth(i)
            self.assertEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertTrue(e in SummerMonth)
            self.assertTrue(type(e) is SummerMonth)

    def test_programatic_function_unicode(self):
        SummerMonth = Enum('SummerMonth', unicode('june july august'))
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate(unicode('june july august').split()):
            i += 1
            e = SummerMonth(i)
            self.assertEqual(int(e.value), i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertTrue(e in SummerMonth)
            self.assertTrue(type(e) is SummerMonth)

    def test_programatic_function_unicode_list(self):
        SummerMonth = Enum('SummerMonth', [unicode('june'), unicode('july'), unicode('august')])
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate(unicode('june july august').split()):
            i += 1
            e = SummerMonth(i)
            self.assertEqual(int(e.value), i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertTrue(e in SummerMonth)
            self.assertTrue(type(e) is SummerMonth)

    def test_programatic_function_unicode_iterable(self):
        SummerMonth = Enum(
                'SummerMonth',
                ((unicode('june'), 1), (unicode('july'), 2), (unicode('august'), 3))
                )
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate(unicode('june july august').split()):
            i += 1
            e = SummerMonth(i)
            self.assertEqual(int(e.value), i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertTrue(e in SummerMonth)
            self.assertTrue(type(e) is SummerMonth)

    def test_programatic_function_from_unicode_dict(self):
        SummerMonth = Enum(
                'SummerMonth',
                dict(((unicode('june'), 1), (unicode('july'), 2), (unicode('august'), 3)))
                )
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        if pyver < 3.0:
            self.assertEqual(
                    [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                    lst,
                    )
        for i, month in enumerate(unicode('june july august').split()):
            i += 1
            e = SummerMonth(i)
            self.assertEqual(int(e.value), i)
            self.assertNotEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertTrue(e in SummerMonth)
            self.assertTrue(type(e) is SummerMonth)

    def test_programatic_function_unicode_type(self):
        SummerMonth = Enum('SummerMonth', unicode('june july august'), type=int)
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate(unicode('june july august').split()):
            i += 1
            e = SummerMonth(i)
            self.assertEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertTrue(e in SummerMonth)
            self.assertTrue(type(e) is SummerMonth)

    def test_programatic_function_unicode_type_from_subclass(self):
        SummerMonth = IntEnum('SummerMonth', unicode('june july august'))
        lst = list(SummerMonth)
        self.assertEqual(len(lst), len(SummerMonth))
        self.assertEqual(len(SummerMonth), 3, SummerMonth)
        self.assertEqual(
                [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                lst,
                )
        for i, month in enumerate(unicode('june july august').split()):
            i += 1
            e = SummerMonth(i)
            self.assertEqual(e, i)
            self.assertEqual(e.name, month)
            self.assertTrue(e in SummerMonth)
            self.assertTrue(type(e) is SummerMonth)

    def test_programmatic_function_unicode_class(self):
        if pyver < 3.0:
            class_names = unicode('SummerMonth'), 'S\xfcmm\xe9rM\xf6nth'.decode('latin1')
        else:
            class_names = 'SummerMonth', 'S\xfcmm\xe9rM\xf6nth'
        for i, class_name in enumerate(class_names):
            if pyver < 3.0 and i == 1:
                self.assertRaises(TypeError, Enum, class_name, unicode('june july august'))
            else:
                SummerMonth = Enum(class_name, unicode('june july august'))
                lst = list(SummerMonth)
                self.assertEqual(len(lst), len(SummerMonth))
                self.assertEqual(len(SummerMonth), 3, SummerMonth)
                self.assertEqual(
                        [SummerMonth.june, SummerMonth.july, SummerMonth.august],
                        lst,
                        )
                for i, month in enumerate(unicode('june july august').split()):
                    i += 1
                    e = SummerMonth(i)
                    self.assertEqual(e.value, i)
                    self.assertEqual(e.name, month)
                    self.assertTrue(e in SummerMonth)
                    self.assertTrue(type(e) is SummerMonth)

    def test_subclassing(self):
        if isinstance(Name, Exception):
            raise Name
        self.assertEqual(Name.BDFL, 'Guido van Rossum')
        self.assertTrue(Name.BDFL, Name('Guido van Rossum'))
        self.assertTrue(Name.BDFL is getattr(Name, 'BDFL'))
        test_pickle_dump_load(self.assertTrue, Name.BDFL)

    def test_extending(self):
        def bad_extension():
            class Color(Enum):
                red = 1
                green = 2
                blue = 3
            class MoreColor(Color):
                cyan = 4
                magenta = 5
                yellow = 6
        self.assertRaises(TypeError, bad_extension)

    def test_exclude_methods(self):
        class whatever(Enum):
            this = 'that'
            these = 'those'
            def really(self):
                return 'no, not %s' % self.value
        self.assertFalse(type(whatever.really) is whatever)
        self.assertEqual(whatever.this.really(), 'no, not that')

    def test_wrong_inheritance_order(self):
        def wrong_inherit():
            class Wrong(Enum, str):
                NotHere = 'error before this point'
        self.assertRaises(TypeError, wrong_inherit)

    def test_intenum_transitivity(self):
        class number(IntEnum):
            one = 1
            two = 2
            three = 3
        class numero(IntEnum):
            uno = 1
            dos = 2
            tres = 3
        self.assertEqual(number.one, numero.uno)
        self.assertEqual(number.two, numero.dos)
        self.assertEqual(number.three, numero.tres)

    def test_introspection(self):
        class Number(IntEnum):
            one = 100
            two = 200
        self.assertTrue(Number.one._member_type_ is int)
        self.assertTrue(Number._member_type_ is int)
        class String(str, Enum):
            yarn = 'soft'
            rope = 'rough'
            wire = 'hard'
        self.assertTrue(String.yarn._member_type_ is str)
        self.assertTrue(String._member_type_ is str)
        class Plain(Enum):
            vanilla = 'white'
            one = 1
        self.assertTrue(Plain.vanilla._member_type_ is object)
        self.assertTrue(Plain._member_type_ is object)

    def test_wrong_enum_in_call(self):
        class Monochrome(Enum):
            black = 0
            white = 1
        class Gender(Enum):
            male = 0
            female = 1
        self.assertRaises(ValueError, Monochrome, Gender.male)

    def test_wrong_enum_in_mixed_call(self):
        class Monochrome(IntEnum):
            black = 0
            white = 1
        class Gender(Enum):
            male = 0
            female = 1
        self.assertRaises(ValueError, Monochrome, Gender.male)

    def test_mixed_enum_in_call_1(self):
        class Monochrome(IntEnum):
            black = 0
            white = 1
        class Gender(IntEnum):
            male = 0
            female = 1
        self.assertTrue(Monochrome(Gender.female) is Monochrome.white)

    def test_mixed_enum_in_call_2(self):
        class Monochrome(Enum):
            black = 0
            white = 1
        class Gender(IntEnum):
            male = 0
            female = 1
        self.assertTrue(Monochrome(Gender.male) is Monochrome.black)

    def test_flufl_enum(self):
        class Fluflnum(Enum):
            def __int__(self):
                return int(self.value)
        class MailManOptions(Fluflnum):
            option1 = 1
            option2 = 2
            option3 = 3
        self.assertEqual(int(MailManOptions.option1), 1)

    def test_no_such_enum_member(self):
        class Color(Enum):
            red = 1
            green = 2
            blue = 3
        self.assertRaises(ValueError, Color, 4)
        self.assertRaises(KeyError, Color.__getitem__, 'chartreuse')

    def test_new_repr(self):
        class Color(Enum):
            red = 1
            green = 2
            blue = 3
            def __repr__(self):
                return "don't you just love shades of %s?" % self.name
        self.assertEqual(
                repr(Color.blue),
                "don't you just love shades of blue?",
                )

    def test_inherited_repr(self):
        class MyEnum(Enum):
            def __repr__(self):
                return "My name is %s." % self.name
        class MyIntEnum(int, MyEnum):
            this = 1
            that = 2
            theother = 3
        self.assertEqual(repr(MyIntEnum.that), "My name is that.")

    def test_multiple_mixin_mro(self):
        class auto_enum(EnumMeta):
            def __new__(metacls, cls, bases, classdict):
                original_dict = classdict
                classdict = enum._EnumDict()
                for k, v in original_dict.items():
                    classdict[k] = v
                temp = type(classdict)()
                names = set(classdict._member_names)
                i = 0
                for k in classdict._member_names:
                    v = classdict[k]
                    if v == ():
                        v = i
                    else:
                        i = v
                    i += 1
                    temp[k] = v
                for k, v in classdict.items():
                    if k not in names:
                        temp[k] = v
                return super(auto_enum, metacls).__new__(
                        metacls, cls, bases, temp)

        AutoNumberedEnum = auto_enum('AutoNumberedEnum', (Enum,), {})

        AutoIntEnum = auto_enum('AutoIntEnum', (IntEnum,), {})

        class TestAutoNumber(AutoNumberedEnum):
            a = ()
            b = 3
            c = ()

        class TestAutoInt(AutoIntEnum):
            a = ()
            b = 3
            c = ()

    def test_subclasses_with_getnewargs(self):
        class NamedInt(int):
            __qualname__ = 'NamedInt'  # needed for pickle protocol 4
            def __new__(cls, *args):
                _args = args
                if len(args) < 1:
                    raise TypeError("name and value must be specified")
                name, args = args[0], args[1:]
                self = int.__new__(cls, *args)
                self._intname = name
                self._args = _args
                return self
            def __getnewargs__(self):
                return self._args
            @property
            def __name__(self):
                return self._intname
            def __repr__(self):
                # repr() is updated to include the name and type info
                return "%s(%r, %s)" % (type(self).__name__,
                                             self.__name__,
                                             int.__repr__(self))
            def __str__(self):
                # str() is unchanged, even if it relies on the repr() fallback
                base = int
                base_str = base.__str__
                if base_str.__objclass__ is object:
                    return base.__repr__(self)
                return base_str(self)
            # for simplicity, we only define one operator that
            # propagates expressions
            def __add__(self, other):
                temp = int(self) + int( other)
                if isinstance(self, NamedInt) and isinstance(other, NamedInt):
                    return NamedInt(
                        '(%s + %s)' % (self.__name__, other.__name__),
                        temp )
                else:
                    return temp

        class NEI(NamedInt, Enum):
            __qualname__ = 'NEI'  # needed for pickle protocol 4
            x = ('the-x', 1)
            y = ('the-y', 2)

        self.assertTrue(NEI.__new__ is Enum.__new__)
        self.assertEqual(repr(NEI.x + NEI.y), "NamedInt('(the-x + the-y)', 3)")
        globals()['NamedInt'] = NamedInt
        globals()['NEI'] = NEI
        NI5 = NamedInt('test', 5)
        self.assertEqual(NI5, 5)
        test_pickle_dump_load(self.assertTrue, NI5, 5)
        self.assertEqual(NEI.y.value, 2)
        test_pickle_dump_load(self.assertTrue, NEI.y)

    if pyver >= 3.4:
        def test_subclasses_with_getnewargs_ex(self):
            class NamedInt(int):
                __qualname__ = 'NamedInt'       # needed for pickle protocol 4
                def __new__(cls, *args):
                    _args = args
                    if len(args) < 2:
                        raise TypeError("name and value must be specified")
                    name, args = args[0], args[1:]
                    self = int.__new__(cls, *args)
                    self._intname = name
                    self._args = _args
                    return self
                def __getnewargs_ex__(self):
                    return self._args, {}
                @property
                def __name__(self):
                    return self._intname
                def __repr__(self):
                    # repr() is updated to include the name and type info
                    return "{}({!r}, {})".format(type(self).__name__,
                                                 self.__name__,
                                                 int.__repr__(self))
                def __str__(self):
                    # str() is unchanged, even if it relies on the repr() fallback
                    base = int
                    base_str = base.__str__
                    if base_str.__objclass__ is object:
                        return base.__repr__(self)
                    return base_str(self)
                # for simplicity, we only define one operator that
                # propagates expressions
                def __add__(self, other):
                    temp = int(self) + int( other)
                    if isinstance(self, NamedInt) and isinstance(other, NamedInt):
                        return NamedInt(
                            '({0} + {1})'.format(self.__name__, other.__name__),
                            temp )
                    else:
                        return temp

            class NEI(NamedInt, Enum):
                __qualname__ = 'NEI'      # needed for pickle protocol 4
                x = ('the-x', 1)
                y = ('the-y', 2)


            self.assertIs(NEI.__new__, Enum.__new__)
            self.assertEqual(repr(NEI.x + NEI.y), "NamedInt('(the-x + the-y)', 3)")
            globals()['NamedInt'] = NamedInt
            globals()['NEI'] = NEI
            NI5 = NamedInt('test', 5)
            self.assertEqual(NI5, 5)
            test_pickle_dump_load(self.assertEqual, NI5, 5, protocol=(4, HIGHEST_PROTOCOL))
            self.assertEqual(NEI.y.value, 2)
            test_pickle_dump_load(self.assertTrue, NEI.y, protocol=(4, HIGHEST_PROTOCOL))

    def test_subclasses_with_reduce(self):
        class NamedInt(int):
            __qualname__ = 'NamedInt'       # needed for pickle protocol 4
            def __new__(cls, *args):
                _args = args
                if len(args) < 1:
                    raise TypeError("name and value must be specified")
                name, args = args[0], args[1:]
                self = int.__new__(cls, *args)
                self._intname = name
                self._args = _args
                return self
            def __reduce__(self):
                return self.__class__, self._args
            @property
            def __name__(self):
                return self._intname
            def __repr__(self):
                # repr() is updated to include the name and type info
                return "%s(%r, %s)" % (type(self).__name__,
                                             self.__name__,
                                             int.__repr__(self))
            def __str__(self):
                # str() is unchanged, even if it relies on the repr() fallback
                base = int
                base_str = base.__str__
                if base_str.__objclass__ is object:
                    return base.__repr__(self)
                return base_str(self)
            # for simplicity, we only define one operator that
            # propagates expressions
            def __add__(self, other):
                temp = int(self) + int( other)
                if isinstance(self, NamedInt) and isinstance(other, NamedInt):
                    return NamedInt(
                        '(%s + %s)' % (self.__name__, other.__name__),
                        temp )
                else:
                    return temp

        class NEI(NamedInt, Enum):
            __qualname__ = 'NEI'      # needed for pickle protocol 4
            x = ('the-x', 1)
            y = ('the-y', 2)


        self.assertTrue(NEI.__new__ is Enum.__new__)
        self.assertEqual(repr(NEI.x + NEI.y), "NamedInt('(the-x + the-y)', 3)")
        globals()['NamedInt'] = NamedInt
        globals()['NEI'] = NEI
        NI5 = NamedInt('test', 5)
        self.assertEqual(NI5, 5)
        test_pickle_dump_load(self.assertEqual, NI5, 5)
        self.assertEqual(NEI.y.value, 2)
        test_pickle_dump_load(self.assertTrue, NEI.y)

    def test_subclasses_with_reduce_ex(self):
        class NamedInt(int):
            __qualname__ = 'NamedInt'       # needed for pickle protocol 4
            def __new__(cls, *args):
                _args = args
                if len(args) < 1:
                    raise TypeError("name and value must be specified")
                name, args = args[0], args[1:]
                self = int.__new__(cls, *args)
                self._intname = name
                self._args = _args
                return self
            def __reduce_ex__(self, proto):
                return self.__class__, self._args
            @property
            def __name__(self):
                return self._intname
            def __repr__(self):
                # repr() is updated to include the name and type info
                return "%s(%r, %s)" % (type(self).__name__,
                                             self.__name__,
                                             int.__repr__(self))
            def __str__(self):
                # str() is unchanged, even if it relies on the repr() fallback
                base = int
                base_str = base.__str__
                if base_str.__objclass__ is object:
                    return base.__repr__(self)
                return base_str(self)
            # for simplicity, we only define one operator that
            # propagates expressions
            def __add__(self, other):
                temp = int(self) + int( other)
                if isinstance(self, NamedInt) and isinstance(other, NamedInt):
                    return NamedInt(
                        '(%s + %s)' % (self.__name__, other.__name__),
                        temp )
                else:
                    return temp

        class NEI(NamedInt, Enum):
            __qualname__ = 'NEI'      # needed for pickle protocol 4
            x = ('the-x', 1)
            y = ('the-y', 2)


        self.assertTrue(NEI.__new__ is Enum.__new__)
        self.assertEqual(repr(NEI.x + NEI.y), "NamedInt('(the-x + the-y)', 3)")
        globals()['NamedInt'] = NamedInt
        globals()['NEI'] = NEI
        NI5 = NamedInt('test', 5)
        self.assertEqual(NI5, 5)
        test_pickle_dump_load(self.assertEqual, NI5, 5)
        self.assertEqual(NEI.y.value, 2)
        test_pickle_dump_load(self.assertTrue, NEI.y)

    def test_subclasses_without_direct_pickle_support(self):
        class NamedInt(int):
            __qualname__ = 'NamedInt'
            def __new__(cls, *args):
                _args = args
                name, args = args[0], args[1:]
                if len(args) == 0:
                    raise TypeError("name and value must be specified")
                self = int.__new__(cls, *args)
                self._intname = name
                self._args = _args
                return self
            @property
            def __name__(self):
                return self._intname
            def __repr__(self):
                # repr() is updated to include the name and type info
                return "%s(%r, %s)" % (type(self).__name__,
                                             self.__name__,
                                             int.__repr__(self))
            def __str__(self):
                # str() is unchanged, even if it relies on the repr() fallback
                base = int
                base_str = base.__str__
                if base_str.__objclass__ is object:
                    return base.__repr__(self)
                return base_str(self)
            # for simplicity, we only define one operator that
            # propagates expressions
            def __add__(self, other):
                temp = int(self) + int( other)
                if isinstance(self, NamedInt) and isinstance(other, NamedInt):
                    return NamedInt(
                        '(%s + %s)' % (self.__name__, other.__name__),
                        temp )
                else:
                    return temp

        class NEI(NamedInt, Enum):
            __qualname__ = 'NEI'
            x = ('the-x', 1)
            y = ('the-y', 2)

        self.assertTrue(NEI.__new__ is Enum.__new__)
        self.assertEqual(repr(NEI.x + NEI.y), "NamedInt('(the-x + the-y)', 3)")
        globals()['NamedInt'] = NamedInt
        globals()['NEI'] = NEI
        NI5 = NamedInt('test', 5)
        self.assertEqual(NI5, 5)
        self.assertEqual(NEI.y.value, 2)
        test_pickle_exception(self.assertRaises, TypeError, NEI.x)
        test_pickle_exception(self.assertRaises, PicklingError, NEI)

    def test_subclasses_without_direct_pickle_support_using_name(self):
        class NamedInt(int):
            __qualname__ = 'NamedInt'
            def __new__(cls, *args):
                _args = args
                name, args = args[0], args[1:]
                if len(args) == 0:
                    raise TypeError("name and value must be specified")
                self = int.__new__(cls, *args)
                self._intname = name
                self._args = _args
                return self
            @property
            def __name__(self):
                return self._intname
            def __repr__(self):
                # repr() is updated to include the name and type info
                return "%s(%r, %s)" % (type(self).__name__,
                                             self.__name__,
                                             int.__repr__(self))
            def __str__(self):
                # str() is unchanged, even if it relies on the repr() fallback
                base = int
                base_str = base.__str__
                if base_str.__objclass__ is object:
                    return base.__repr__(self)
                return base_str(self)
            # for simplicity, we only define one operator that
            # propagates expressions
            def __add__(self, other):
                temp = int(self) + int( other)
                if isinstance(self, NamedInt) and isinstance(other, NamedInt):
                    return NamedInt(
                        '(%s + %s)' % (self.__name__, other.__name__),
                        temp )
                else:
                    return temp

        class NEI(NamedInt, Enum):
            __qualname__ = 'NEI'
            x = ('the-x', 1)
            y = ('the-y', 2)
            def __reduce_ex__(self, proto):
                return getattr, (self.__class__, self._name_)

        self.assertTrue(NEI.__new__ is Enum.__new__)
        self.assertEqual(repr(NEI.x + NEI.y), "NamedInt('(the-x + the-y)', 3)")
        globals()['NamedInt'] = NamedInt
        globals()['NEI'] = NEI
        NI5 = NamedInt('test', 5)
        self.assertEqual(NI5, 5)
        self.assertEqual(NEI.y.value, 2)
        test_pickle_dump_load(self.assertTrue, NEI.y)
        test_pickle_dump_load(self.assertTrue, NEI)

    def test_tuple_subclass(self):
        class SomeTuple(tuple, Enum):
            __qualname__ = 'SomeTuple'
            first = (1, 'for the money')
            second = (2, 'for the show')
            third = (3, 'for the music')
        self.assertTrue(type(SomeTuple.first) is SomeTuple)
        self.assertTrue(isinstance(SomeTuple.second, tuple))
        self.assertEqual(SomeTuple.third, (3, 'for the music'))
        globals()['SomeTuple'] = SomeTuple
        test_pickle_dump_load(self.assertTrue, SomeTuple.first)

    def test_duplicate_values_give_unique_enum_items(self):
        class AutoNumber(Enum):
            __order__ = 'enum_m enum_d enum_y'
            enum_m = ()
            enum_d = ()
            enum_y = ()
            def __new__(cls):
                value = len(cls.__members__) + 1
                obj = object.__new__(cls)
                obj._value_ = value
                return obj
            def __int__(self):
                return int(self._value_)
        self.assertEqual(int(AutoNumber.enum_d), 2)
        self.assertEqual(AutoNumber.enum_y.value, 3)
        self.assertTrue(AutoNumber(1) is AutoNumber.enum_m)
        self.assertEqual(
            list(AutoNumber),
            [AutoNumber.enum_m, AutoNumber.enum_d, AutoNumber.enum_y],
            )

    def test_inherited_new_from_enhanced_enum(self):
        class AutoNumber2(Enum):
            def __new__(cls):
                value = len(cls.__members__) + 1
                obj = object.__new__(cls)
                obj._value_ = value
                return obj
            def __int__(self):
                return int(self._value_)
        class Color(AutoNumber2):
            __order__ = 'red green blue'
            red = ()
            green = ()
            blue = ()
        self.assertEqual(len(Color), 3, "wrong number of elements: %d (should be %d)" % (len(Color), 3))
        self.assertEqual(list(Color), [Color.red, Color.green, Color.blue])
        if pyver >= 3.0:
            self.assertEqual(list(map(int, Color)), [1, 2, 3])

    def test_inherited_new_from_mixed_enum(self):
        class AutoNumber3(IntEnum):
            def __new__(cls):
                value = len(cls.__members__) + 1
                obj = int.__new__(cls, value)
                obj._value_ = value
                return obj
        class Color(AutoNumber3):
            red = ()
            green = ()
            blue = ()
        self.assertEqual(len(Color), 3, "wrong number of elements: %d (should be %d)" % (len(Color), 3))
        Color.red
        Color.green
        Color.blue

    def test_equality(self):
        class AlwaysEqual:
            def __eq__(self, other):
                return True
        class OrdinaryEnum(Enum):
            a = 1
        self.assertEqual(AlwaysEqual(), OrdinaryEnum.a)
        self.assertEqual(OrdinaryEnum.a, AlwaysEqual())

    def test_ordered_mixin(self):
        class OrderedEnum(Enum):
            def __ge__(self, other):
                if self.__class__ is other.__class__:
                    return self._value_ >= other._value_
                return NotImplemented
            def __gt__(self, other):
                if self.__class__ is other.__class__:
                    return self._value_ > other._value_
                return NotImplemented
            def __le__(self, other):
                if self.__class__ is other.__class__:
                    return self._value_ <= other._value_
                return NotImplemented
            def __lt__(self, other):
                if self.__class__ is other.__class__:
                    return self._value_ < other._value_
                return NotImplemented
        class Grade(OrderedEnum):
            __order__ = 'A B C D F'
            A = 5
            B = 4
            C = 3
            D = 2
            F = 1
        self.assertEqual(list(Grade), [Grade.A, Grade.B, Grade.C, Grade.D, Grade.F])
        self.assertTrue(Grade.A > Grade.B)
        self.assertTrue(Grade.F <= Grade.C)
        self.assertTrue(Grade.D < Grade.A)
        self.assertTrue(Grade.B >= Grade.B)

    def test_extending2(self):
        def bad_extension():
            class Shade(Enum):
                def shade(self):
                    print(self.name)
            class Color(Shade):
                red = 1
                green = 2
                blue = 3
            class MoreColor(Color):
                cyan = 4
                magenta = 5
                yellow = 6
        self.assertRaises(TypeError, bad_extension)

    def test_extending3(self):
        class Shade(Enum):
            def shade(self):
                return self.name
        class Color(Shade):
            def hex(self):
                return '%s hexlified!' % self.value
        class MoreColor(Color):
            cyan = 4
            magenta = 5
            yellow = 6
        self.assertEqual(MoreColor.magenta.hex(), '5 hexlified!')

    def test_no_duplicates(self):
        def bad_duplicates():
            class UniqueEnum(Enum):
                def __init__(self, *args):
                    cls = self.__class__
                    if any(self.value == e.value for e in cls):
                        a = self.name
                        e = cls(self.value).name
                        raise ValueError(
                                "aliases not allowed in UniqueEnum:  %r --> %r"
                                % (a, e)
                                )
            class Color(UniqueEnum):
                red = 1
                green = 2
                blue = 3
            class Color(UniqueEnum):
                red = 1
                green = 2
                blue = 3
                grene = 2
        self.assertRaises(ValueError, bad_duplicates)

    def test_init(self):
        class Planet(Enum):
            MERCURY = (3.303e+23, 2.4397e6)
            VENUS   = (4.869e+24, 6.0518e6)
            EARTH   = (5.976e+24, 6.37814e6)
            MARS    = (6.421e+23, 3.3972e6)
            JUPITER = (1.9e+27,   7.1492e7)
            SATURN  = (5.688e+26, 6.0268e7)
            URANUS  = (8.686e+25, 2.5559e7)
            NEPTUNE = (1.024e+26, 2.4746e7)
            def __init__(self, mass, radius):
                self.mass = mass       # in kilograms
                self.radius = radius   # in meters
            @property
            def surface_gravity(self):
                # universal gravitational constant  (m3 kg-1 s-2)
                G = 6.67300E-11
                return G * self.mass / (self.radius * self.radius)
        self.assertEqual(round(Planet.EARTH.surface_gravity, 2), 9.80)
        self.assertEqual(Planet.EARTH.value, (5.976e+24, 6.37814e6))

    def test_nonhash_value(self):
        class AutoNumberInAList(Enum):
            def __new__(cls):
                value = [len(cls.__members__) + 1]
                obj = object.__new__(cls)
                obj._value_ = value
                return obj
        class ColorInAList(AutoNumberInAList):
            __order__ = 'red green blue'
            red = ()
            green = ()
            blue = ()
        self.assertEqual(list(ColorInAList), [ColorInAList.red, ColorInAList.green, ColorInAList.blue])
        self.assertEqual(ColorInAList.red.value, [1])
        self.assertEqual(ColorInAList([1]), ColorInAList.red)

    def test_conflicting_types_resolved_in_new(self):
        class LabelledIntEnum(int, Enum):
            def __new__(cls, *args):
                value, label = args
                obj = int.__new__(cls, value)
                obj.label = label
                obj._value_ = value
                return obj

        class LabelledList(LabelledIntEnum):
            unprocessed = (1, "Unprocessed")
            payment_complete = (2, "Payment Complete")

        self.assertEqual(list(LabelledList), [LabelledList.unprocessed, LabelledList.payment_complete])
        self.assertEqual(LabelledList.unprocessed, 1)
        self.assertEqual(LabelledList(1), LabelledList.unprocessed)

    def test_empty_with_functional_api(self):
        empty = enum.IntEnum('Foo', {})
        self.assertEqual(len(empty), 0)


class TestUnique(unittest.TestCase):
    """2.4 doesn't allow class decorators, use function syntax."""

    def test_unique_clean(self):
        class Clean(Enum):
            one = 1
            two = 'dos'
            tres = 4.0
        unique(Clean)
        class Cleaner(IntEnum):
            single = 1
            double = 2
            triple = 3
        unique(Cleaner)

    def test_unique_dirty(self):
        try:
            class Dirty(Enum):
                __order__ = 'one two tres'
                one = 1
                two = 'dos'
                tres = 1
            unique(Dirty)
        except ValueError:
            exc = sys.exc_info()[1]
            message = exc.args[0]
        self.assertTrue('tres -> one' in message)

        try:
            class Dirtier(IntEnum):
                __order__ = 'single double triple turkey'
                single = 1
                double = 1
                triple = 3
                turkey = 3
            unique(Dirtier)
        except ValueError:
            exc = sys.exc_info()[1]
            message = exc.args[0]
        self.assertTrue('double -> single' in message)
        self.assertTrue('turkey -> triple' in message)


class TestMe(unittest.TestCase):

    pass

if __name__ == '__main__':
    unittest.main()
