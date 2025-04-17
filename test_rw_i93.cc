#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>
#include "rw_i93.h"

using ::testing::_;
using ::testing::AnyNumber;

class MockCallback {
public:
    MOCK_METHOD(void, Callback, (uint8_t event, tRW_DATA* p_data), ());
};

MockCallback mockCallback;

void MockCallbackFunction(uint8_t event, tRW_DATA* p_data) {
    mockCallback.Callback(event, p_data);
}

uint8_t dummy_data[256];

class RwI93Test : public ::testing::Test {
protected:
    void SetUp() override {
        memset(&rw_cb, 0, sizeof(rw_cb));
        rw_cb.p_cback = MockCallbackFunction;
        memset(dummy_data, 0xAB, sizeof(dummy_data));
    }
    void TearDown() override {}

    NFC_HDR* MakeResp(uint8_t* data, uint16_t len, uint16_t offset = 0) {
        NFC_HDR* resp = (NFC_HDR*)malloc(sizeof(NFC_HDR) + len + 8);
        memset(resp, 0, sizeof(NFC_HDR) + len + 8);
        resp->len = len;
        resp->offset = offset;
        if (data && len > 0) {
            memcpy((uint8_t*)(resp + 1) + offset, data, len);
        }
        return resp;
    }
};

// --- Initial State Tests ---
TEST_F(RwI93Test, TestEmptyResponse) {
    NFC_HDR resp = {};
    resp.len = 0;

    EXPECT_CALL(mockCallback, Callback(RW_I93_NDEF_UPDATE_CPLT_EVT, _)).Times(0);

    rw_i93_sm_update_ndef(&resp);
    EXPECT_EQ(rw_cb.tcb.i93.state, RW_I93_STATE_IDLE);
}

TEST_F(RwI93Test, TestInvalidBlockSize) {
    NFC_HDR resp = {};
    resp.len = 10;
    rw_cb.tcb.i93.block_size = I93_MAX_BLOCK_LENGH + 1;

    EXPECT_CALL(mockCallback, Callback(RW_I93_NDEF_UPDATE_CPLT_EVT, _)).Times(0);

    rw_i93_sm_update_ndef(&resp);
    EXPECT_EQ(rw_cb.tcb.i93.state, RW_I93_STATE_IDLE);
}

// --- Error Flag Tests ---
TEST_F(RwI93Test, TestErrorFlagDetected_StdChipInlay) {
    NFC_HDR resp = {};
    uint8_t data[] = {I93_FLAG_ERROR_DETECTED, I93_ERROR_CODE_BLOCK_FAIL_TO_WRITE};
    resp.len = sizeof(data);
    memcpy((uint8_t*)(&resp + 1), data, sizeof(data));

    rw_cb.tcb.i93.product_version = RW_I93_TAG_IT_HF_I_STD_CHIP_INLAY;

    EXPECT_CALL(mockCallback, Callback(RW_I93_NDEF_UPDATE_CPLT_EVT, _)).Times(0);

    rw_i93_sm_update_ndef(&resp);
    EXPECT_EQ(rw_cb.tcb.i93.state, RW_I93_STATE_IDLE);
}

TEST_F(RwI93Test, TestErrorFlagDetected_PlusInlay_IgnoreError) {
    NFC_HDR resp = {};
    uint8_t data[] = {I93_FLAG_ERROR_DETECTED, I93_ERROR_CODE_BLOCK_FAIL_TO_WRITE};
    resp.len = sizeof(data);
    memcpy((uint8_t*)(&resp + 1), data, sizeof(data));

    rw_cb.tcb.i93.product_version = RW_I93_TAG_IT_HF_I_PLUS_INLAY;

    EXPECT_CALL(mockCallback, Callback(RW_I93_NDEF_UPDATE_CPLT_EVT, _)).Times(0);

    rw_i93_sm_update_ndef(&resp);
    EXPECT_EQ(rw_cb.tcb.i93.state, RW_I93_STATE_IDLE);
}

TEST_F(RwI93Test, TestErrorFlagDetected_PlusInlay_HandleError) {
    NFC_HDR resp = {};
    uint8_t data[] = {I93_FLAG_ERROR_DETECTED, 0x00};
    resp.len = sizeof(data);
    memcpy((uint8_t*)(&resp + 1), data, sizeof(data));

    rw_cb.tcb.i93.product_version = RW_I93_TAG_IT_HF_I_PLUS_INLAY;

    EXPECT_CALL(mockCallback, Callback(RW_I93_NDEF_UPDATE_CPLT_EVT, _)).Times(0);

    rw_i93_sm_update_ndef(&resp);
    EXPECT_EQ(rw_cb.tcb.i93.state, RW_I93_STATE_IDLE);
}

// --- RW_I93_SUBSTATE_RESET_LEN Tests ---
TEST_F(RwI93Test, CoversResetLen_AllBranches) {
    rw_cb.tcb.i93.sub_state = RW_I93_SUBSTATE_RESET_LEN;
    rw_cb.tcb.i93.block_size = 4;
    rw_cb.tcb.i93.ndef_tlv_start_offset = 3;
    rw_cb.tcb.i93.ndef_length = 2;
    rw_cb.tcb.i93.rw_length = 0;
    rw_cb.tcb.i93.p_update_data = dummy_data;
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00};
    NFC_HDR* resp = MakeResp(data, 5);

    EXPECT_CALL(mockCallback, Callback(_, _)).Times(AnyNumber());
    rw_i93_sm_update_ndef(resp);
    free(resp);
}

TEST_F(RwI93Test, CoversResetLen_LengthLessThanOffset) {
    rw_cb.tcb.i93.sub_state = RW_I93_SUBSTATE_RESET_LEN;
    rw_cb.tcb.i93.block_size = 4;
    rw_cb.tcb.i93.ndef_tlv_start_offset = 3;
    uint8_t data[] = {0x00, 0x00};
    NFC_HDR* resp = MakeResp(data, 2);

    EXPECT_CALL(mockCallback, Callback(_, _)).Times(0);
    rw_i93_sm_update_ndef(resp);
    free(resp);
}

TEST_F(RwI93Test, CoversResetLen_LengthLessThanBlockSize) {
    rw_cb.tcb.i93.sub_state = RW_I93_SUBSTATE_RESET_LEN;
    rw_cb.tcb.i93.block_size = 4;
    rw_cb.tcb.i93.ndef_tlv_start_offset = 1;
    uint8_t data[] = {0x00, 0x00, 0x00};
    NFC_HDR* resp = MakeResp(data, 3);

    EXPECT_CALL(mockCallback, Callback(_, _)).Times(0);
    rw_i93_sm_update_ndef(resp);
    free(resp);
}

// --- RW_I93_SUBSTATE_WRITE_NDEF Tests ---
TEST_F(RwI93Test, CoversWriteNdef_AllBranches) {
    rw_cb.tcb.i93.sub_state = RW_I93_SUBSTATE_WRITE_NDEF;
    rw_cb.tcb.i93.block_size = 4;
    rw_cb.tcb.i93.num_block = 10;
    rw_cb.tcb.i93.rw_offset = 0;
    rw_cb.tcb.i93.ndef_length = 5;
    rw_cb.tcb.i93.rw_length = 0;
    rw_cb.tcb.i93.p_update_data = dummy_data;
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00};
    NFC_HDR* resp = MakeResp(data, 5);

    EXPECT_CALL(mockCallback, Callback(_, _)).Times(AnyNumber());
    rw_i93_sm_update_ndef(resp);
    free(resp);
}

TEST_F(RwI93Test, CoversWriteNdef_LastBlockOfTLV) {
    rw_cb.tcb.i93.sub_state = RW_I93_SUBSTATE_WRITE_NDEF;
    rw_cb.tcb.i93.block_size = 4;
    rw_cb.tcb.i93.num_block = 10;
    rw_cb.tcb.i93.rw_offset = 0;
    rw_cb.tcb.i93.ndef_length = 5;
    rw_cb.tcb.i93.rw_length = 2;
    rw_cb.tcb.i93.p_update_data = dummy_data;
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00};
    NFC_HDR* resp = MakeResp(data, 5);

    EXPECT_CALL(mockCallback, Callback(_, _)).Times(AnyNumber());
    rw_i93_sm_update_ndef(resp);
    free(resp);
}

TEST_F(RwI93Test, CoversWriteNdef_NextBlockOfTLV) {
    rw_cb.tcb.i93.sub_state = RW_I93_SUBSTATE_WRITE_NDEF;
    rw_cb.tcb.i93.block_size = 4;
    rw_cb.tcb.i93.num_block = 10;
    rw_cb.tcb.i93.rw_offset = 4;
    rw_cb.tcb.i93.ndef_length = 5;
    rw_cb.tcb.i93.rw_length = 5;
    rw_cb.tcb.i93.ndef_tlv_last_offset = 3;
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00};
    NFC_HDR* resp = MakeResp(data, 5);

    EXPECT_CALL(mockCallback, Callback(_, _)).Times(AnyNumber());
    rw_i93_sm_update_ndef(resp);
    free(resp);
}

TEST_F(RwI93Test, CoversWriteNdef_FinishedWriting) {
    rw_cb.tcb.i93.sub_state = RW_I93_SUBSTATE_WRITE_NDEF;
    rw_cb.tcb.i93.block_size = 4;
    rw_cb.tcb.i93.num_block = 10;
    rw_cb.tcb.i93.rw_offset = 4;
    rw_cb.tcb.i93.ndef_length = 5;
    rw_cb.tcb.i93.rw_length = 5;
    rw_cb.tcb.i93.ndef_tlv_start_offset = 3;
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00};
    NFC_HDR* resp = MakeResp(data, 5);

    EXPECT_CALL(mockCallback, Callback(_, _)).Times(AnyNumber());
    rw_i93_sm_update_ndef(resp);
    free(resp);
}

TEST_F(RwI93Test, CoversWriteNdef_NoMoreDataToWrite) {
    rw_cb.tcb.i93.sub_state = RW_I93_SUBSTATE_WRITE_NDEF;
    rw_cb.tcb.i93.block_size = 4;
    rw_cb.tcb.i93.num_block = 1;
    rw_cb.tcb.i93.rw_offset = 4;
    rw_cb.tcb.i93.ndef_length = 5;
    rw_cb.tcb.i93.rw_length = 5;
    rw_cb.tcb.i93.ndef_tlv_start_offset = 3;
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00};
    NFC_HDR* resp = MakeResp(data, 5);

    EXPECT_CALL(mockCallback, Callback(_, _)).Times(AnyNumber());
    rw_i93_sm_update_ndef(resp);
    free(resp);
}

// --- RW_I93_SUBSTATE_UPDATE_LEN Tests ---
TEST_F(RwI93Test, CoversUpdateLen_AllBranches) {
    rw_cb.tcb.i93.sub_state = RW_I93_SUBSTATE_UPDATE_LEN;
    rw_cb.tcb.i93.block_size = 4;
    rw_cb.tcb.i93.rw_offset = 0;
    rw_cb.tcb.i93.ndef_length = 0x100;
    rw_cb.tcb.i93.rw_length = 3;
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00};
    NFC_HDR* resp = MakeResp(data, 5);

    EXPECT_CALL(mockCallback, Callback(_, _)).Times(AnyNumber());
    rw_i93_sm_update_ndef(resp);
    free(resp);
}

TEST_F(RwI93Test, CoversUpdateLen_LengthZero) {
    rw_cb.tcb.i93.sub_state = RW_I93_SUBSTATE_UPDATE_LEN;
    rw_cb.tcb.i93.block_size = 4;
    rw_cb.tcb.i93.rw_offset = 0;
    rw_cb.tcb.i93.ndef_length = 0x100;
    rw_cb.tcb.i93.rw_length = 3;
    uint8_t data[] = {0x00};
    NFC_HDR* resp = MakeResp(data, 0);

    EXPECT_CALL(mockCallback, Callback(_, _)).Times(AnyNumber());
    rw_i93_sm_update_ndef(resp);
    free(resp);
}

TEST_F(RwI93Test, CoversUpdateLen_LengthGreaterThanBlockSize) {
    rw_cb.tcb.i93.sub_state = RW_I93_SUBSTATE_UPDATE_LEN;
    rw_cb.tcb.i93.block_size = 4;
    rw_cb.tcb.i93.rw_offset = 0;
    rw_cb.tcb.i93.ndef_length = 0x100;
    rw_cb.tcb.i93.rw_length = 3;
    uint8_t data[] = {0x00, 0x00, 0x00};
    NFC_HDR* resp = MakeResp(data, 3);

    EXPECT_CALL(mockCallback, Callback(_, _)).Times(AnyNumber());
    rw_i93_sm_update_ndef(resp);
    free(resp);
}

TEST_F(RwI93Test, CoversUpdateLen_UpdateComplete) {
    rw_cb.tcb.i93.sub_state = RW_I93_SUBSTATE_UPDATE_LEN;
    rw_cb.tcb.i93.block_size = 4;
    rw_cb.tcb.i93.rw_offset = 0;
    rw_cb.tcb.i93.ndef_length = 0x100;
    rw_cb.tcb.i93.rw_length = 0;
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00};
    NFC_HDR* resp = MakeResp(data, 5);

    EXPECT_CALL(mockCallback, Callback(_, _)).Times(AnyNumber());
    rw_i93_sm_update_ndef(resp);
    free(resp);
}

// --- Additional Test Cases for Increased Coverage ---
TEST_F(RwI93Test, CoversWriteNdef_NullUpdateData) {
    rw_cb.tcb.i93.sub_state = RW_I93_SUBSTATE_WRITE_NDEF;
    rw_cb.tcb.i93.block_size = 4;
    rw_cb.tcb.i93.num_block = 10;
    rw_cb.tcb.i93.rw_offset = 0;
    rw_cb.tcb.i93.ndef_length = 5;
    rw_cb.tcb.i93.rw_length = 0;
    rw_cb.tcb.i93.p_update_data = nullptr;
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00};
    NFC_HDR* resp = MakeResp(data, 5);

    EXPECT_CALL(mockCallback, Callback(_, _)).Times(AnyNumber());
    rw_i93_sm_update_ndef(resp);
    free(resp);
}

TEST_F(RwI93Test, CoversUpdateLen_NullResponseData) {
    rw_cb.tcb.i93.sub_state = RW_I93_SUBSTATE_UPDATE_LEN;
    rw_cb.tcb.i93.block_size = 4;
    rw_cb.tcb.i93.rw_offset = 0;
    rw_cb.tcb.i93.ndef_length = 0x100;
    rw_cb.tcb.i93.rw_length = 3;
    NFC_HDR* resp = MakeResp(nullptr, 0);

    EXPECT_CALL(mockCallback, Callback(_, _)).Times(AnyNumber());
    rw_i93_sm_update_ndef(resp);
    free(resp);
}

TEST_F(RwI93Test, CoversResetLen_NullResponseData) {
    rw_cb.tcb.i93.sub_state = RW_I93_SUBSTATE_RESET_LEN;
    rw_cb.tcb.i93.block_size = 4;
    rw_cb.tcb.i93.ndef_tlv_start_offset = 3;
    rw_cb.tcb.i93.ndef_length = 2;
    rw_cb.tcb.i93.rw_length = 0;
    rw_cb.tcb.i93.p_update_data = dummy_data;
    NFC_HDR* resp = MakeResp(nullptr, 0);

    EXPECT_CALL(mockCallback, Callback(_, _)).Times(AnyNumber());
    rw_i93_sm_update_ndef(resp);
    free(resp);
}

TEST_F(RwI93Test, CoversAllStates_InvalidSubState) {
    rw_cb.tcb.i93.sub_state = 99;
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00};
    NFC_HDR* resp = MakeResp(data, 5);

    EXPECT_CALL(mockCallback, Callback(_, _)).Times(AnyNumber());
    rw_i93_sm_update_ndef(resp);
    free(resp);
}
