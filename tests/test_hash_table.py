import ctypes
import string
import random
import pytest
LIB_PATH = "./tests/libs/hash_table.a"

lib = ctypes.CDLL(LIB_PATH)



# typedef struct Entry {
#   char *key;
#   void *value;
#   struct Entry *next;
# } HashEntry;

class HashEntry(ctypes.Structure):
        pass
HashEntry._fields_ = [("key",ctypes.c_char_p),
                ("value",ctypes.c_void_p),
                ("next",ctypes.POINTER(HashEntry))]
# typedef struct {
#   int capacity;
#   int entry_count;
#   HashEntry **entries;
#
# } HashTable;

class HashTable(ctypes.Structure):
    _fields_ = [("capacity",ctypes.c_int),
                ("entry_count",ctypes.c_int),
                ("entries",ctypes.POINTER(ctypes.POINTER(HashEntry)))]


c_hash_table_p = ctypes.POINTER(HashTable)

# HashTable *initTable(int capacity);
initTable = lib.initTable
initTable.argtypes = [ctypes.c_int]
initTable.restype =  c_hash_table_p

# void add(char *key, const void *value, int val_size, HashTable *table);
add = lib.add
add.argtypes = [ctypes.c_char_p,ctypes.c_void_p,ctypes.c_int,c_hash_table_p]
add.restype = None

# void removeKey(char *key, HashTable *table);
removeKey = lib.removeKey
removeKey.argtypes = [ctypes.c_char_p,c_hash_table_p]
removeKey.restype = None

# void *get(char *key, HashTable *table);
get = lib.get
get.argtypes = [ctypes.c_char_p,c_hash_table_p]
get.restype = ctypes.c_void_p

# char *getAsString(char *key, HashTable *table);
getAsString = lib.getAsString
getAsString.argtypes = [ctypes.c_char_p,c_hash_table_p]
getAsString.restype = ctypes.c_char_p

# float getAsFloat(char *key, HashTable *table);

getAsFloat = lib.getAsFloat
getAsFloat.argtypes = [ctypes.c_char_p,c_hash_table_p]
getAsFloat.restype = ctypes.c_float

# int getAsInt(char *key, HashTable *table);
getAsInt = lib.getAsInt
getAsInt.argtypes = [ctypes.c_char_p,c_hash_table_p]
getAsInt.restype = ctypes.c_int

# void freeTable(HashTable **hashtable);
freeTable = lib.freeTable
freeTable.argtypes = [ctypes.POINTER(c_hash_table_p)]
freeTable.restype =  None

TABLE_SIZE = 10

def addStr(k:str,v:str,table):
    key = ctypes.c_char_p(k.encode())
    val = ctypes.c_char_p(v.encode())
    size = ctypes.c_int(len(v)+1)
    add(key,val,size,table)

def addInt(k:str,v:int,table:HashTable):
    key = ctypes.c_char_p(k.encode())
    val = ctypes.c_int(v)
    size = ctypes.sizeof(val)
    add(key,ctypes.byref(val),size,table)

def addFloat(k:str,v:float,table:HashTable):
    key = ctypes.c_char_p(k.encode())
    val = ctypes.c_float(v)
    size = ctypes.sizeof(val)
    add(key,ctypes.byref(val),size,table)



@pytest.fixture
def table():
    t = initTable(TABLE_SIZE)
    yield t
    freeTable(ctypes.byref(t))

def test_init_and_free():
    table = initTable(TABLE_SIZE)
    assert table.contents.capacity == TABLE_SIZE
    assert table.contents.entry_count == 0
    freeTable(ctypes.byref(table))
    assert not table

def test_add(table):
    keys = set({})
    for i in range(5):
        let = f"{random.choice(string.ascii_letters)}".encode()
        key = ctypes.c_char_p(let)
        val = ctypes.c_int(i+100)
        add(key,ctypes.byref(val),ctypes.sizeof(val),table)
        keys.add(let)
    assert table.contents.entry_count == len(keys)

@pytest.mark.parametrize("key,val",[("Key1","Val1"),("Key2","Val2"),("Key1",100)])
def test_get(table,key,val):
    ret = None
    c_key_p = ctypes.c_char_p(key.encode())
    if(type(val) == str):
        addStr(key,val,table)
        ret = getAsString(c_key_p,table)
        ret = ret.decode()
    elif(type(val) == int):
        addInt(key,val,table)
        ret = getAsInt(c_key_p,table)
    assert ret == val


def test_grow(table):
    keys = ["a","b","c","d","e","f","g","h"]
    assert table.contents.capacity == TABLE_SIZE 
    for i in range(len(keys)):
        addStr(keys[i],keys[i],table)
    assert table.contents.capacity == TABLE_SIZE*2
    for i in range(len(keys)):
        ret = getAsString(keys[i].encode(),table)
        assert ret.decode() == keys[i]

def test_N_elements(table):
    keys = set({})
    N = 1000000
    while(len(keys) < N):
        let = ''.join(random.choices(string.ascii_letters, k=6))
        addStr(let,let,table)
        keys.add(let)
    assert table.contents.entry_count == len(keys)
    for key in keys:
        ret = getAsString(key.encode(),table)
        assert ret.decode() == key
        removeKey(key.encode(),table)
    assert table.contents.entry_count == 0


def test_remove(table):
    key = ctypes.c_char_p(b"Key")
    val = ctypes.c_char_p(b"VALUE")
    size = ctypes.c_int(len(val.value if val.value else ""))
    add(key,val,size,table)
    assert table.contents.entry_count == 1
    removeKey(key,table)
    ret = getAsInt(key,table)
    assert not ret

