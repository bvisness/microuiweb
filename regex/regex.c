#include "regex.h"

void Regex_AddUnionMember(Regex* regex, NoUnionEx* ex) {
    regex->UnionMembers[regex->NumUnionMembers] = ex;
    regex->NumUnionMembers++;
}

void Regex_RemoveUnionMember(Regex* regex, int index) {
    regex->NumUnionMembers--;
    for (int i = index; i < regex->NumUnionMembers; i++) {
        regex->UnionMembers[i] = regex->UnionMembers[i + 1];
    }

    // TODO: Free the deleted thing??
}

void NoUnionEx_AddUnit(NoUnionEx* ex, struct Unit* unit, int index) {
    assert(ex->NumUnits < MAX_UNITS);
    assert(unit->Parent == NULL);

    unit->Parent = ex;
    unit->Index = index;

    if (index < 0) {
        ex->Units[ex->NumUnits] = unit;
        ex->NumUnits++;

        unit->Index = ex->NumUnits-1;

        goto checkAndReturn;
    }

    // shift units over
    for (int i = ex->NumUnits - 1; i >= index; i--) {
        ex->Units[i + 1] = ex->Units[i];
        ex->Units[i + 1]->Index = i + 1;
    }

    ex->Units[index] = unit;
    ex->NumUnits++;

checkAndReturn:
    assert(unit->Parent == ex);
}

Unit* NoUnionEx_RemoveUnit(NoUnionEx* ex, int index) {
    assert(ex->NumUnits > 0);

    if (index == -1) {
        ex->NumUnits--;
        Unit* unit = ex->Units[ex->NumUnits];
        unit->Parent = NULL;
        return unit;
    }

    Unit* unit = ex->Units[index];

    ex->NumUnits--;
    for (int i = index; i < ex->NumUnits; i++) {
        ex->Units[i] = ex->Units[i + 1];
        ex->Units[i]->Index = i;
    }

    unit->Parent = NULL;
    return unit;
}

void NoUnionEx_ReplaceUnits(NoUnionEx* ex, int Start, int End, struct Unit* unit) {
    // TODO: This could someday be more efficient by doing one big shift.
    for (int i = 0; i < End - Start + 1; i++) {
        NoUnionEx_RemoveUnit(ex, Start);
    }

    NoUnionEx_AddUnit(ex, unit, Start);

    // TODO: We can't return all the units that got replaced! What if we need
    // to free them?
}

Unit* Unit_Previous(Unit* unit) {
    if (unit->Index == 0) {
        return NULL;
    }

    return unit->Parent->Units[unit->Index - 1];
}

Unit* Unit_Next(Unit* unit) {
    if (unit->Index + 1 >= unit->Parent->NumUnits) {
        return NULL;
    }

    Unit* next = unit->Parent->Units[unit->Index + 1];
    assert(unit->Parent == next->Parent);

    return next;
}

void Unit_SetRepeatMin(Unit* unit, int val) {
    unit->RepeatMin = val;
    unit->_minbuf = (float) val;
}

void Unit_SetRepeatMax(Unit* unit, int val) {
    unit->RepeatMax = val;
    unit->_maxbuf = (float) val;
}

int Unit_IsNonSingular(Unit* unit) {
    return Unit_IsSkip(unit) || Unit_IsRepeat(unit);
}

int Unit_IsSkip(Unit* unit) {
    return unit->RepeatMin < 1;
}

int Unit_IsRepeat(Unit* unit) {
    return unit->RepeatMax != 1;
}

int Unit_ShouldShowLeftHandle(Unit* unit) {
    if (Unit_Previous(unit)) {
        return 0;
    }

    if (unit->IsSelected) {
        return 1;
    } else {
        return (
            unit->IsLeftWireHover
            || unit->IsContentHover
            || (unit->IsRightWireHover && unit->IsShowingLeftHandle)
            || unit->IsWireDragOrigin
        );
    }
}

int Unit_ShouldShowRightHandle(Unit* unit) {
    Unit* next = Unit_Next(unit);
    Unit* prev = Unit_Previous(unit);

    if (!unit->IsSelected && (next && next->IsSelected)) {
        return 1;
    } else if (unit->IsSelected && !(next && next->IsSelected)) {
        return 1;
    } else if (!unit->IsSelected) {
        int rightWireActive = unit->IsRightWireHover || unit->IsWireDragOrigin;
        return (
            unit->IsContentHover
            || unit->IsLeftWireHover
            || rightWireActive
            || (prev && prev->IsRightWireHover && unit->IsShowingRightHandle)
            || (next && next->IsContentHover)
            || (next && next->IsRightWireHover && unit->IsShowingRightHandle)
        );
    }

    return 0;
}

void Set_AddItem(Set* set, struct SetItem* item, int index) {
    assert(set->NumItems < MAX_SET_ITEMS);

    if (index < 0) {
        set->Items[set->NumItems] = item;
        set->NumItems++;

        return;
    }

    // shift units over
    for (int i = set->NumItems - 1; i >= index; i--) {
        set->Items[i + 1] = set->Items[i];
    }

    set->Items[index] = item;
    set->NumItems++;
}

struct SetItem* Set_RemoveItem(Set* set, int index) {
    assert(set->NumItems > 0);

    if (index == -1) {
        set->NumItems--;
        SetItem* item = set->Items[set->NumItems];
        return item;
    }

    SetItem* item = set->Items[index];

    set->NumItems--;
    for (int i = index; i < set->NumItems; i++) {
        set->Items[i] = set->Items[i + 1];
    }

    return item;
}


char* toString_Regex(char* base, Regex* regex);
char* toString_NoUnionEx(char* base, NoUnionEx* ex);
char* toString_Unit(char* base, Unit* unit);
char* toString_UnitContents(char* base, UnitContents* contents);
char* toString_Group(char* base, Group* group);
char* toString_Set(char* base, Set* set);
char* toString_SetItem(char* base, SetItem* item);
char* toString_SetItemRange(char* base, SetItemRange* range);
char* toString_Special(char* base, Special* special);
char* toString_LitChar(char* base, LitChar* c);
char* toString_MetaChar(char* base, MetaChar* c);

/*
Enum Functions
*/

const char* RE_CONTENTS_ToString(int v) {
    switch (v) {
        case RE_CONTENTS_LITCHAR: return "Character";
        case RE_CONTENTS_METACHAR: return "Metacharacter";
        case RE_CONTENTS_SPECIAL: return "Special";
        case RE_CONTENTS_SET: return "Set";
        case RE_CONTENTS_GROUP: return "Group";
    }

    return "UNKNOWN RE_CONTENTS";
}

const char* RE_SETITEM_ToString(int v){
    switch (v) {
        case RE_SETITEM_LITCHAR: return "Character";
        case RE_SETITEM_RANGE: return "Range";
    }

    return "UNKNOWN RE_SETITEM";
}


/*
ToString
*/

char toStringBuf[2048];
char* ToString(Regex* regex) {
    char* finalBase = toString_Regex(toStringBuf, regex);
    *finalBase = 0;

    return toStringBuf;
}

char* writeLiteral(char* base, const char* literal) {
    size_t len = strlen(literal);
    memcpy(base, literal, len);
    return base + len;
}

char* toString_Regex(char* base, Regex* regex) {
    if (regex == NULL) {
        return base;
    }

    for (int i = 0; i < regex->NumUnionMembers; i++) {
        if (i > 0) {
            base = writeLiteral(base, "|");
        }
        base = toString_NoUnionEx(base, regex->UnionMembers[i]);
    }

    return base;
}

char* toString_NoUnionEx(char* base, NoUnionEx* ex) {
    if (ex == NULL) {
        return base;
    }

    for (int i = 0; i < ex->NumUnits; i++) {
        base = toString_Unit(base, ex->Units[i]);
    }

    return base;
}

char* toString_Unit(char* base, Unit* unit) {
    if (unit == NULL) {
        return base;
    }

    base = toString_UnitContents(base, unit->Contents);

    if (unit->RepeatMin == 1 && unit->RepeatMax == 1) {
        // do nothing, this is the default
    } else if (unit->RepeatMin == 0 && unit->RepeatMax == 0) {
        base = writeLiteral(base, "*");
    } else if (unit->RepeatMin == 1 && unit->RepeatMax == 0) {
        base = writeLiteral(base, "+");
    } else if (unit->RepeatMin == 0 && unit->RepeatMax == 1) {
        base = writeLiteral(base, "?");
    } else {
        base = writeLiteral(base, "{");
        base += sprintf(base, "%d", unit->RepeatMin);
        base = writeLiteral(base, ",");
        if (unit->RepeatMax >= 0) {
            base += sprintf(base, "%d", unit->RepeatMax);
        }
        base = writeLiteral(base, "}");
    }

    return base;
}

char* toString_UnitContents(char* base, UnitContents* contents) {
    if (contents == NULL) {
        return base;
    }

    switch (contents->Type) {
        case RE_CONTENTS_GROUP: {
            base = toString_Group(base, contents->Group);
        } break;
        case RE_CONTENTS_SET: {
            base = toString_Set(base, contents->Set);
        } break;
        case RE_CONTENTS_SPECIAL: {
            base = toString_Special(base, contents->Special);
        } break;
        case RE_CONTENTS_LITCHAR: {
            base = toString_LitChar(base, contents->LitChar);
        } break;
        case RE_CONTENTS_METACHAR: {
            base = toString_MetaChar(base, contents->MetaChar);
        } break;
    }

    return base;
}

char* toString_Group(char* base, Group* group) {
    if (group == NULL) {
        return base;
    }

    base = writeLiteral(base, "(");
    base = toString_Regex(base, group->Regex);
    base = writeLiteral(base, ")");

    return base;
}

char* toString_Set(char* base, Set* set) {
    if (set == NULL) {
        return base;
    }

    base = writeLiteral(base, "[");
    if (set->IsNegative) {
        base = writeLiteral(base, "^");
    }
    for (int i = 0; i < set->NumItems; i++) {
        base = toString_SetItem(base, set->Items[i]);
    }
    base = writeLiteral(base, "]");

    return base;
}

char* toString_SetItem(char* base, SetItem* item) {
    if (item == NULL) {
        return base;
    }

    switch (item->Type) {
        case RE_SETITEM_LITCHAR: {
            base = toString_LitChar(base, item->LitChar);
        } break;
        case RE_SETITEM_RANGE: {
            base = toString_SetItemRange(base, item->Range);
        } break;
    }

    return base;
}

char* toString_SetItemRange(char* base, SetItemRange* range) {
    if (range == NULL) {
        return base;
    }

    base = toString_LitChar(base, range->Min);
    base = writeLiteral(base, "-");
    base = toString_LitChar(base, range->Max);

    return base;
}

char* toString_Special(char* base, Special* special) {
    if (special == NULL) {
        return base;
    }

    switch (special->Type) {
        case RE_SPECIAL_ANY: {
            base = writeLiteral(base, ".");
        } break;
        case RE_SPECIAL_STRINGSTART: {
            base = writeLiteral(base, "^");
        } break;
        case RE_SPECIAL_STRINGEND: {
            base = writeLiteral(base, "$");
        } break;
    }

    return base;
}

char* toString_LitChar(char* base, LitChar* c) {
    if (c == NULL) {
        return base;
    }

    *base = c->C;
    base++;

    return base;
}

char* toString_MetaChar(char* base, MetaChar* c) {
    if (c == NULL) {
        return base;
    }

    base = writeLiteral(base, "\\");

    *base = c->C;
    base++;

    return base;
}

