#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "phNxpNciHal_ext.h"

// Ensure the include paths for Google Test and Google Mock are correctly set in your build system.
// Add the necessary include paths for "hardware/nfc.h" and other dependencies in your build configuration.

// Mock definitions for external dependencies
bool_t gsIsFwRecoveryRequired = false;
bool nfc_debug_enabled = false;
bool bEnableMfcExtns = false;
bool bEnableMfcReader = false;
uint8_t icode_detected = 0x00;
uint8_t icode_send_eof = 0x00;
uint8_t ee_disc_done = 0x00;
uint32_t wFwVerRsp = 0;
uint16_t wFwVer = 0;
phNxpNciHal_Control_t nxpncihal_ctrl = {};
phNxpNciProfile_Control_t nxpprofile_ctrl = {};
PowerTrackerHandle gPowerTrackerHandle = {};
const char* core_reset_ntf_count_prop_name = "core_reset_ntf_count";
    MOCK_METHOD(void, AnalyzeMfcResp, (uint8_t*, uint16_t*), (override));
class MockNxpMfcReaderInstance {
public:
    MOCK_METHOD(void, AnalyzeMfcResp, (uint8_t*, uint16_t*), ());
};

MockNxpMfcReaderInstance NxpMfcReaderInstance;
    MOCK_METHOD(void, Log, (uint8_t*, uint16_t, LogEventType), (override));
class MockPhNxpEventLogger {
public:
    MOCK_METHOD(void, Log, (uint8_t*, uint16_t, LogEventType), ());
    static MockPhNxpEventLogger& GetInstance() {
        static MockPhNxpEventLogger instance;
        return instance;
    }
};
    MOCK_METHOD(void, UpdateICTempStatus, (uint8_t*, uint16_t), (override));
class MockPhNxpTempMgr {
public:
    MOCK_METHOD(void, UpdateICTempStatus, (uint8_t*, uint16_t), ());
    static MockPhNxpTempMgr& GetInstance() {
        static MockPhNxpTempMgr instance;
        return instance;
    }
};

// Test fixture
class PhNxpNciHalExtTest : public ::testing::Test {
protected:
    void SetUp() override {
        gsIsFwRecoveryRequired = false;
        bEnableMfcExtns = false;
        icode_detected = 0x00;
        icode_send_eof = 0x00;
        ee_disc_done = 0x00;
        nxpprofile_ctrl.profile_type = NFC_FORUM_PROFILE;
    }
};

// Test cases
TEST_F(PhNxpNciHalExtTest, TestInvalidLengthForRfIntfActivatedNtf) {
    uint8_t p_ntf[] = {0x61, 0x05};
    uint16_t p_len = 6;

    NFCSTATUS status = phNxpNciHal_process_ext_rsp(p_ntf, &p_len);

    EXPECT_EQ(status, NFCSTATUS_FAILED);
}

TEST_F(PhNxpNciHalExtTest, TestFelicaReaderModeEnabled) {
    uint8_t p_ntf[] = {0x61, 0x05, 0x00, 0x00, 0x01, 0x05, 0x02};
    uint16_t p_len = sizeof(p_ntf);
    gFelicaReaderMode = true;

    NFCSTATUS status = phNxpNciHal_process_ext_rsp(p_ntf, &p_len);

    EXPECT_EQ(p_ntf[5], 0x03);
    EXPECT_EQ(status, NFCSTATUS_SUCCESS);
}

TEST_F(PhNxpNciHalExtTest, TestMifareExtnsEnabled) {
    uint8_t p_ntf[] = {0x61, 0x05, 0x00, 0x00, 0x80, 0x80};
    uint16_t p_len = sizeof(p_ntf);

    NFCSTATUS status = phNxpNciHal_process_ext_rsp(p_ntf, &p_len);

    EXPECT_TRUE(bEnableMfcExtns);
    EXPECT_EQ(status, NFCSTATUS_SUCCESS);
}

TEST_F(PhNxpNciHalExtTest, TestIso15693Notification) {
    uint8_t p_ntf[] = {0x61, 0x05, 0x15, 0x00, 0x01, 0x06, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint16_t p_len = sizeof(p_ntf);

    NFCSTATUS status = phNxpNciHal_process_ext_rsp(p_ntf, &p_len);

    EXPECT_EQ(icode_detected, 0x01);
    EXPECT_EQ(p_ntf[21], 0x01);
    EXPECT_EQ(p_ntf[22], 0x01);
    EXPECT_EQ(status, NFCSTATUS_SUCCESS);
}

TEST_F(PhNxpNciHalExtTest, TestCoreGenericError) {
    uint8_t p_ntf[] = {0x60, 0x07, 0x01, CORE_GENERIC_ERR_CURRENT_NTF};
    uint16_t p_len = sizeof(p_ntf);

    NFCSTATUS status = phNxpNciHal_process_ext_rsp(p_ntf, &p_len);

    EXPECT_TRUE(gsIsFwRecoveryRequired);
    EXPECT_EQ(status, NFCSTATUS_FAILED);
}

TEST_F(PhNxpNciHalExtTest, TestTemperatureStatusNotification) {
    uint8_t p_ntf[] = {0x6F, NCI_OID_SYSTEM_TERMPERATURE_INFO_NTF, 0x06};
    uint16_t p_len = sizeof(p_ntf);

    EXPECT_CALL(MockPhNxpTempMgr::GetInstance(), UpdateICTempStatus(p_ntf, p_len)).Times(1);

    NFCSTATUS status = phNxpNciHal_process_ext_rsp(p_ntf, &p_len);

    EXPECT_EQ(status, NFCSTATUS_SUCCESS);
}

TEST_F(PhNxpNciHalExtTest, TestEmptyNotification) {
    uint8_t p_ntf[] = {};
    uint16_t p_len = 0;

    NFCSTATUS status = phNxpNciHal_process_ext_rsp(p_ntf, &p_len);

    EXPECT_EQ(status, NFCSTATUS_FAILED);
}

TEST_F(PhNxpNciHalExtTest, TestInvalidNotificationType) {
    uint8_t p_ntf[] = {0xFF, 0xFF};
    uint16_t p_len = sizeof(p_ntf);

    NFCSTATUS status = phNxpNciHal_process_ext_rsp(p_ntf, &p_len);

    EXPECT_EQ(status, NFCSTATUS_FAILED);
}

TEST_F(PhNxpNciHalExtTest, TestValidNotificationWithEdgeCaseData) {
    uint8_t p_ntf[] = {0x61, 0x05, 0x00, 0x00, 0xFF, 0xFF};
    uint16_t p_len = sizeof(p_ntf);

    NFCSTATUS status = phNxpNciHal_process_ext_rsp(p_ntf, &p_len);

    EXPECT_EQ(status, NFCSTATUS_SUCCESS);
}

TEST_F(PhNxpNciHalExtTest, TestNotificationWithMaxLength) {
    uint8_t p_ntf[255] = {0x61, 0x05};
    uint16_t p_len = sizeof(p_ntf);

    NFCSTATUS status = phNxpNciHal_process_ext_rsp(p_ntf, &p_len);

    EXPECT_EQ(status, NFCSTATUS_SUCCESS);
}
