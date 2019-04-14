#include "../redisearch_api.h"
#include <gtest/gtest.h>
#include <set>
#include <string>

#define DOCID1 "doc1"
#define DOCID2 "doc2"
#define FIELD_NAME_1 "text1"
#define FIELD_NAME_2 "text2"
#define NUMERIC_FIELD_NAME "num"
#define TAG_FIELD_NAME1 "tag1"
#define TAG_FIELD_NAME2 "tag2"

REDISEARCH_API_INIT_SYMBOLS();

class LLApiTest : public ::testing::Test {
  virtual void SetUp() {
    RediSearch_Initialize();
  }

  virtual void TearDown() {
  }
};

TEST_F(LLApiTest, testGetVersion) {
  ASSERT_EQ(RediSearch_GetCApiVersion(), REDISEARCH_CAPI_VERSION);
}

static std::vector<std::string> getResultsCommon(RSIndex* index, RSResultsIterator* it,
                                                 bool expectEmpty) {
  std::vector<std::string> ret;
  if (expectEmpty) {
    EXPECT_TRUE(it == NULL);
  } else {
    EXPECT_FALSE(it == NULL);
  }

  if (!it) {
    goto done;
  }

  while (true) {
    size_t n = 0;
    auto cur = RediSearch_ResultsIteratorNext(it, index, &n);
    if (cur == NULL) {
      break;
    }
    ret.push_back(std::string((const char*)cur, n));
  }

done:
  if (it) {
    RediSearch_ResultsIteratorFree(it);
  }
  return ret;
}

static std::vector<std::string> getResults(RSIndex* index, RSQueryNode* qn,
                                           bool expectEmpty = false) {
  auto it = RediSearch_GetResultsIterator(qn, index);
  return getResultsCommon(index, it, expectEmpty);
}

static std::vector<std::string> getResults(RSIndex* index, const char* s,
                                           bool expectEmpty = false) {
  auto it = RediSearch_IterateQuery(index, s, strlen(s), NULL);
  return getResultsCommon(index, it, expectEmpty);
}

TEST_F(LLApiTest, testAddDocumentTextField) {
  // creating the index
  RSIndex* index = RediSearch_CreateIndex("index", NULL, NULL);

  // adding text field to the index
  RediSearch_CreateField(index, FIELD_NAME_1, RSFLDTYPE_FULLTEXT, RSFLDOPT_NONE);

  // adding document to the index
  RSDoc* d = RediSearch_CreateDocument(DOCID1, strlen(DOCID1), 1.0, NULL);
  RediSearch_DocumentAddFieldCString(d, FIELD_NAME_1, "some test to index", RSFLDTYPE_DEFAULT);
  RediSearch_SpecAddDocument(index, d);

  // searching on the index
#define SEARCH_TERM "index"
  RSQNode* qn = RediSearch_CreateTokenNode(index, FIELD_NAME_1, SEARCH_TERM);
  RSResultsIterator* iter = RediSearch_GetResultsIterator(qn, index);

  size_t len;
  const char* id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID1);
  id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResultsIteratorFree(iter);

  // test prefix search
  qn = RediSearch_CreatePrefixNode(index, FIELD_NAME_1, "in");
  iter = RediSearch_GetResultsIterator(qn, index);

  id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID1);
  id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResultsIteratorFree(iter);

  // search with no results
  qn = RediSearch_CreatePrefixNode(index, FIELD_NAME_1, "nn");
  iter = RediSearch_GetResultsIterator(qn, index);
  ASSERT_FALSE(iter);

  // adding another text field
  RediSearch_CreateField(index, FIELD_NAME_2, RSFLDTYPE_FULLTEXT, RSFLDOPT_NONE);

  // adding document to the index with both fields
  d = RediSearch_CreateDocument(DOCID2, strlen(DOCID2), 1.0, NULL);
  RediSearch_DocumentAddFieldCString(d, FIELD_NAME_1, "another indexing testing",
                                     RSFLDTYPE_DEFAULT);
  RediSearch_DocumentAddFieldCString(d, FIELD_NAME_2, "another indexing testing",
                                     RSFLDTYPE_DEFAULT);
  RediSearch_SpecAddDocument(index, d);

  // test prefix search, should return both documents now
  qn = RediSearch_CreatePrefixNode(index, FIELD_NAME_1, "in");
  iter = RediSearch_GetResultsIterator(qn, index);

  id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID1);
  id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID2);
  id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResultsIteratorFree(iter);

  // test prefix search on second field, should return only second document
  qn = RediSearch_CreatePrefixNode(index, FIELD_NAME_2, "an");
  iter = RediSearch_GetResultsIterator(qn, index);

  id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID2);
  id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResultsIteratorFree(iter);

  // delete the second document
  int ret = RediSearch_DropDocument(index, DOCID2, strlen(DOCID2));
  ASSERT_TRUE(ret);

  // searching again, make sure there is no results
  qn = RediSearch_CreatePrefixNode(index, FIELD_NAME_2, "an");
  iter = RediSearch_GetResultsIterator(qn, index);

  id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResultsIteratorFree(iter);
  RediSearch_DropIndex(index);
}

TEST_F(LLApiTest, testAddDocumentNumericField) {
  // creating the index
  RSIndex* index = RediSearch_CreateIndex("index", NULL, NULL);

  // adding text field to the index
  RediSearch_CreateNumericField(index, NUMERIC_FIELD_NAME);

  // adding document to the index
  RSDoc* d = RediSearch_CreateDocument(DOCID1, strlen(DOCID1), 1.0, NULL);
  RediSearch_DocumentAddFieldNumber(d, NUMERIC_FIELD_NAME, 20, RSFLDTYPE_DEFAULT);
  RediSearch_SpecAddDocument(index, d);

  // searching on the index
  RSQNode* qn = RediSearch_CreateNumericNode(index, NUMERIC_FIELD_NAME, 30, 10, 0, 0);
  RSResultsIterator* iter = RediSearch_GetResultsIterator(qn, index);
  ASSERT_TRUE(iter != NULL);

  size_t len;
  const char* id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID1);
  id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResultsIteratorFree(iter);
  RediSearch_DropIndex(index);
}

TEST_F(LLApiTest, testAddDocumetTagField) {
  // creating the index
  RSIndex* index = RediSearch_CreateIndex("index", NULL, NULL);

  // adding text field to the index
  RediSearch_CreateTagField(index, TAG_FIELD_NAME1);

  // adding document to the index
#define TAG_VALUE "tag_value"
  RSDoc* d = RediSearch_CreateDocument(DOCID1, strlen(DOCID1), 1.0, NULL);
  RediSearch_DocumentAddFieldCString(d, TAG_FIELD_NAME1, TAG_VALUE, RSFLDTYPE_DEFAULT);
  RediSearch_SpecAddDocument(index, d);

  // searching on the index
  RSQNode* qn = RediSearch_CreateTagNode(index, TAG_FIELD_NAME1);
  RSQNode* tqn = RediSearch_CreateTokenNode(index, NULL, TAG_VALUE);
  RediSearch_QueryNodeAddChild(qn, tqn);
  RSResultsIterator* iter = RediSearch_GetResultsIterator(qn, index);

  size_t len;
  const char* id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID1);
  id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResultsIteratorFree(iter);

  // prefix search on the index
  qn = RediSearch_CreateTagNode(index, TAG_FIELD_NAME1);
  tqn = RediSearch_CreatePrefixNode(index, NULL, "ta");
  RediSearch_QueryNodeAddChild(qn, tqn);
  iter = RediSearch_GetResultsIterator(qn, index);

  id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, DOCID1);
  id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
  ASSERT_STREQ(id, NULL);

  RediSearch_ResultsIteratorFree(iter);
  RediSearch_DropIndex(index);
}

TEST_F(LLApiTest, testPhoneticSearch) {
  // creating the index
  RSIndex* index = RediSearch_CreateIndex("index", NULL, NULL);
  RediSearch_CreateField(index, FIELD_NAME_1, RSFLDTYPE_FULLTEXT, RSFLDOPT_TXTPHONETIC);
  RediSearch_CreateField(index, FIELD_NAME_2, RSFLDTYPE_FULLTEXT, RSFLDOPT_NONE);

  RSDoc* d = RediSearch_CreateDocument(DOCID1, strlen(DOCID1), 1.0, NULL);
  RediSearch_DocumentAddFieldCString(d, FIELD_NAME_1, "felix", RSFLDTYPE_DEFAULT);
  RediSearch_DocumentAddFieldCString(d, FIELD_NAME_2, "felix", RSFLDTYPE_DEFAULT);
  RediSearch_SpecAddDocument(index, d);

  // make sure phonetic search works on field1
  RSQNode* qn = RediSearch_CreateTokenNode(index, FIELD_NAME_1, "phelix");
  auto res = getResults(index, qn);
  ASSERT_EQ(1, res.size());
  ASSERT_EQ(DOCID1, res[0]);

  // make sure phonetic search on field2 do not return results
  qn = RediSearch_CreateTokenNode(index, FIELD_NAME_2, "phelix");
  res = getResults(index, qn, true);
  ASSERT_EQ(0, res.size());
  RediSearch_DropIndex(index);
}

TEST_F(LLApiTest, testMassivePrefix) {
  // creating the index
  RSIndex* index = RediSearch_CreateIndex("index", NULL, NULL);
  RediSearch_CreateTagField(index, TAG_FIELD_NAME1);

  char buff[1024];
  int NUM_OF_DOCS = 1000;
  for (int i = 0; i < NUM_OF_DOCS; ++i) {
    sprintf(buff, "doc%d", i);
    RSDoc* d = RediSearch_CreateDocument(buff, strlen(buff), 1.0, NULL);
    sprintf(buff, "tag-%d", i);
    RediSearch_DocumentAddFieldCString(d, TAG_FIELD_NAME1, buff, RSFLDTYPE_DEFAULT);
    RediSearch_SpecAddDocument(index, d);
  }

  RSQNode* qn = RediSearch_CreateTagNode(index, TAG_FIELD_NAME1);
  RSQNode* pqn = RediSearch_CreatePrefixNode(index, NULL, "tag-");
  RediSearch_QueryNodeAddChild(qn, pqn);
  RSResultsIterator* iter = RediSearch_GetResultsIterator(qn, index);
  ASSERT_TRUE(iter);

  for (size_t i = 0; i < NUM_OF_DOCS; ++i) {
    size_t len;
    const char* id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
    ASSERT_TRUE(id);
  }

  RediSearch_ResultsIteratorFree(iter);
  RediSearch_DropIndex(index);
}

TEST_F(LLApiTest, testRanges) {
  RSIndex* index = RediSearch_CreateIndex("index", NULL, NULL);
  RediSearch_CreateTextField(index, FIELD_NAME_1);
  char buf[] = {"Mark_"};
  size_t nbuf = strlen(buf);
  for (char c = 'a'; c < 'z'; c++) {
    buf[nbuf - 1] = c;
    char did[64];
    sprintf(did, "doc%c", c);
    RSDoc* d = RediSearch_CreateDocument(did, strlen(did), 0, NULL);
    RediSearch_DocumentAddFieldCString(d, FIELD_NAME_1, buf, RSFLDTYPE_DEFAULT);
    RediSearch_SpecAddDocument(index, d);
  }

  RSQNode* qn = RediSearch_CreateLexRangeNode(index, FIELD_NAME_1, "MarkN", "MarkX");
  RSResultsIterator* iter = RediSearch_GetResultsIterator(qn, index);
  ASSERT_FALSE(NULL == iter);
  std::set<std::string> results;
  const char* id;
  size_t nid;
  while ((id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &nid))) {
    std::string idstr(id, nid);
    ASSERT_EQ(results.end(), results.find(idstr));
    results.insert(idstr);
  }

  ASSERT_EQ(10, results.size());
  for (char c = 'n'; c < 'x'; c++) {
    char namebuf[64];
    sprintf(namebuf, "doc%c", c);
    ASSERT_NE(results.end(), results.find(namebuf));
  }
  RediSearch_ResultsIteratorFree(iter);

  // printf("Have %lu ids in range!\n", results.size());
  RediSearch_DropIndex(index);
}

static char buffer[1024];

static int GetValue(void* ctx, const char* fieldName, const void* id, char** strVal,
                    double* doubleVal) {
  *strVal = buffer;
  int numId;
  sscanf((char*)id, "doc%d", &numId);
  if (strcmp(fieldName, TAG_FIELD_NAME1) == 0) {
    sprintf(*strVal, "tag1-%d", numId);
  } else {
    sprintf(*strVal, "tag2-%d", numId);
  }
  return RSVALTYPE_STRING;
}

TEST_F(LLApiTest, testMassivePrefixWithUnsortedSupport) {
  // creating the index
  RSIndex* index = RediSearch_CreateIndex("index", GetValue, NULL);
  RediSearch_CreateTagField(index, TAG_FIELD_NAME1);

  char buff[1024];
  int NUM_OF_DOCS = 10000;
  for (int i = 0; i < NUM_OF_DOCS; ++i) {
    sprintf(buff, "doc%d", i);
    RSDoc* d = RediSearch_CreateDocument(buff, strlen(buff), 1.0, NULL);
    sprintf(buff, "tag-%d", i);
    RediSearch_DocumentAddFieldCString(d, TAG_FIELD_NAME1, buff, RSFLDTYPE_DEFAULT);
    RediSearch_SpecAddDocument(index, d);
  }

  RSQNode* qn = RediSearch_CreateTagNode(index, TAG_FIELD_NAME1);
  RSQNode* pqn = RediSearch_CreatePrefixNode(index, NULL, "tag-");
  RediSearch_QueryNodeAddChild(qn, pqn);
  RSResultsIterator* iter = RediSearch_GetResultsIterator(qn, index);
  ASSERT_TRUE(iter);

  for (size_t i = 0; i < NUM_OF_DOCS; ++i) {
    size_t len;
    const char* id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
    ASSERT_TRUE(id);
  }

  RediSearch_ResultsIteratorFree(iter);
  RediSearch_DropIndex(index);
}

TEST_F(LLApiTest, testPrefixIntersection) {
  // creating the index
  RSIndex* index = RediSearch_CreateIndex("index", GetValue, NULL);
  RediSearch_CreateTagField(index, TAG_FIELD_NAME1);
  RediSearch_CreateTagField(index, TAG_FIELD_NAME2);

  char buff[1024];
  int NUM_OF_DOCS = 1000;
  for (int i = 0; i < NUM_OF_DOCS; ++i) {
    sprintf(buff, "doc%d", i);
    RSDoc* d = RediSearch_CreateDocument(buff, strlen(buff), 1.0, NULL);
    sprintf(buff, "tag1-%d", i);
    RediSearch_DocumentAddFieldCString(d, TAG_FIELD_NAME1, buff, RSFLDTYPE_DEFAULT);
    sprintf(buff, "tag2-%d", i);
    RediSearch_DocumentAddFieldCString(d, TAG_FIELD_NAME2, buff, RSFLDTYPE_DEFAULT);
    RediSearch_SpecAddDocument(index, d);
  }

  RSQNode* qn1 = RediSearch_CreateTagNode(index, TAG_FIELD_NAME1);
  RSQNode* pqn1 = RediSearch_CreatePrefixNode(index, NULL, "tag1-");
  RediSearch_QueryNodeAddChild(qn1, pqn1);
  RSQNode* qn2 = RediSearch_CreateTagNode(index, TAG_FIELD_NAME2);
  RSQNode* pqn2 = RediSearch_CreatePrefixNode(index, NULL, "tag2-");
  RediSearch_QueryNodeAddChild(qn2, pqn2);
  RSQNode* iqn = RediSearch_CreateIntersectNode(index, 0);
  RediSearch_QueryNodeAddChild(iqn, qn1);
  RediSearch_QueryNodeAddChild(iqn, qn2);

  RSResultsIterator* iter = RediSearch_GetResultsIterator(iqn, index);
  ASSERT_TRUE(iter);

  for (size_t i = 0; i < NUM_OF_DOCS; ++i) {
    size_t len;
    const char* id = (const char*)RediSearch_ResultsIteratorNext(iter, index, &len);
    ASSERT_STRNE(id, NULL);
  }

  RediSearch_ResultsIteratorFree(iter);
  RediSearch_DropIndex(index);
}

TEST_F(LLApiTest, testMultitype) {
  RSIndex* index = RediSearch_CreateIndex("index", NULL, NULL);
  auto* f = RediSearch_CreateField(index, "f1", RSFLDTYPE_FULLTEXT, RSFLDOPT_NONE);
  ASSERT_TRUE(f != NULL);
  f = RediSearch_CreateField(index, "f2", RSFLDTYPE_FULLTEXT | RSFLDTYPE_TAG | RSFLDTYPE_NUMERIC,
                             RSFLDOPT_NONE);

  // Add document...
  RSDoc* d = RediSearch_CreateDocumentSimple("doc1");
  RediSearch_DocumentAddFieldCString(d, "f1", "hello", RSFLDTYPE_FULLTEXT);
  RediSearch_DocumentAddFieldCString(d, "f2", "world", RSFLDTYPE_FULLTEXT | RSFLDTYPE_TAG);
  RediSearch_SpecAddDocument(index, d);

  // Done
  // Now search for them...
  auto qn = RediSearch_CreateTokenNode(index, "f1", "hello");
  auto results = getResults(index, qn);
  ASSERT_EQ(1, results.size());
  ASSERT_EQ("doc1", results[0]);
}

TEST_F(LLApiTest, testQueryString) {
  RSIndex* index = RediSearch_CreateIndex("index", NULL, NULL);
  RediSearch_CreateField(index, "ft1", RSFLDTYPE_FULLTEXT, RSFLDOPT_NONE);
  RediSearch_CreateField(index, "ft2", RSFLDTYPE_FULLTEXT, RSFLDOPT_NONE);
  RediSearch_CreateField(index, "n1", RSFLDTYPE_NUMERIC, RSFLDOPT_NONE);
  RediSearch_CreateField(index, "tg1", RSFLDTYPE_TAG, RSFLDOPT_NONE);

  // Insert the documents...
  for (size_t ii = 0; ii < 100; ++ii) {
    char docbuf[1024] = {0};
    sprintf(docbuf, "doc%lu\n", ii);
    Document* d = RediSearch_CreateDocumentSimple(docbuf);
    // Fill with fields..
    sprintf(docbuf, "hello%lu\n", ii);
    RediSearch_DocumentAddFieldCString(d, "ft1", docbuf, RSFLDTYPE_DEFAULT);
    sprintf(docbuf, "world%lu\n", ii);
    RediSearch_DocumentAddFieldCString(d, "ft2", docbuf, RSFLDTYPE_DEFAULT);
    sprintf(docbuf, "tag%lu\n", ii);
    RediSearch_DocumentAddFieldCString(d, "tg1", docbuf, RSFLDTYPE_TAG);
    RediSearch_DocumentAddFieldNumber(d, "n1", ii, RSFLDTYPE_DEFAULT);
    RediSearch_SpecAddDocument(index, d);
  }

  // Issue a query
  auto res = getResults(index, "hello*");
  ASSERT_EQ(100, res.size());

  res = getResults(index, "@ft1:hello*");
  ASSERT_EQ(100, res.size());

  res = getResults(index, "(@ft1:hello1)|(@ft1:hello50)");
  ASSERT_EQ(2, res.size());
}