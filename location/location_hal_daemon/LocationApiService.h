/* Copyright (c) 2018, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LOCATIONAPISERVICE_H
#define LOCATIONAPISERVICE_H

#include <unordered_map>
#include <string>
#include <mutex>

#include <MsgTask.h>
#include <loc_cfg.h>
#include <LocIpc.h>
#ifdef POWERMANAGER_ENABLED
#include <PowerEvtHandler.h>
#endif
#include <location_interface.h>
#include <LocationAPI.h>
#include <LocationApiMsg.h>

#include <LocHalDaemonClientHandler.h>

#undef LOG_TAG
#define LOG_TAG "LocSvc_HalDaemon"

#define SERVICE_NAME "locapiservice"

/******************************************************************************
LocationApiService
******************************************************************************/
class LocationApiService
{
public:
    // singleton instance
    LocationApiService(const LocationApiService&) = delete;
    LocationApiService& operator = (const LocationApiService&) = delete;

    static LocationApiService* getInstance(uint32_t autostart, uint32_t sessiontbfms) {
        if (nullptr == mInstance) {
            mInstance = new LocationApiService(autostart, sessiontbfms);
        }
        return mInstance;
    }

    static void destroy() {
        if (nullptr != mInstance) {
            delete mInstance;
            mInstance = nullptr;
        }
    }

    // APIs can be invoked by IPC
    void processClientMsg(const char* data, uint32_t length);

    // from IPC receiver
    void onListenerReady(bool externalApIpc);

#ifdef POWERMANAGER_ENABLED
    // power event handler
    void onSuspend();
    void onResume();
    void onShutdown();
#endif

    // other APIs
    void deleteClientbyName(const std::string name);

    static std::mutex mMutex;

private:
    // APIs can be invoked to process client's IPC messgage
    void newClient(LocAPIClientRegisterReqMsg*);
    void deleteClient(LocAPIClientDeregisterReqMsg*);

    void startTracking(LocAPIStartTrackingReqMsg*);
    void stopTracking(LocAPIStopTrackingReqMsg*);
    void updateSubscription(LocAPIUpdateCallbacksReqMsg*);
    void updateTrackingOptions(LocAPIUpdateTrackingOptionsReqMsg*);
    void updateNetworkAvailability(bool availability);
    void getGnssEnergyConsumed(const char* clientSocketName);

    void startBatching(LocAPIStartBatchingReqMsg*);
    void stopBatching(LocAPIStopBatchingReqMsg*);
    void updateBatchingOptions(LocAPIUpdateBatchingOptionsReqMsg*);

    void addGeofences(LocAPIAddGeofencesReqMsg*);
    void removeGeofences(LocAPIRemoveGeofencesReqMsg*);
    void modifyGeofences(LocAPIModifyGeofencesReqMsg*);
    void pauseGeofences(LocAPIPauseGeofencesReqMsg*);
    void resumeGeofences(LocAPIResumeGeofencesReqMsg*);

    void pingTest(LocAPIPingTestReqMsg*);

    inline void gnssUpdateConfig(GnssConfig config) {
        mLocationControlApi->gnssUpdateConfig(config);
    }

    inline void gnssDeleteAidingData(GnssAidingData& data) {
        mLocationControlApi->gnssDeleteAidingData(data);
    }

    // Location control API callback
    void onControlResponseCallback(LocationError err, uint32_t id);
    void onControlCollectiveResponseCallback(size_t count, LocationError *errs, uint32_t *ids);
    void onGnssEnergyConsumedCb(uint64_t totalEnergyConsumedSinceFirstBoot);

    LocationApiService(uint32_t autostart, uint32_t sessiontbfms);
    virtual ~LocationApiService();

    // private utilities
    inline LocHalDaemonClientHandler* getClient(const std::string& clientname) {
        // find client from property db
        auto client = mClients.find(clientname);
        if (client == std::end(mClients)) {
            LOC_LOGe("Failed to find client %s", clientname.c_str());
            return nullptr;
        }
        return client->second;
    }

    inline LocHalDaemonClientHandler* getClient(const char* socketName) {
        std::string clientname(socketName);
        return getClient(clientname);
    }

    void checkEnableGnss();

    GnssInterface* getGnssInterface();
    // OSFramework instance
    void createOSFrameworkInstance();
    void destroyOSFrameworkInstance();

#ifdef POWERMANAGER_ENABLED
    // power event observer
    PowerEvtHandler* mPowerEventObserver;
#endif

    // singleton instance
    static LocationApiService *mInstance;

    // IPC interface
    LocIpc mIpc;
    unique_ptr<LocIpcRecver> mBlockingRecver;

    // Client propery database
    std::unordered_map<std::string, LocHalDaemonClientHandler*> mClients;

    // Location Control API interface
    uint32_t mLocationControlId;
    LocationControlCallbacks mControlCallabcks;
    LocationControlAPI *mLocationControlApi;

    // Configration
    const uint32_t mAutoStartGnss;
};

#endif //LOCATIONAPISERVICE_H

