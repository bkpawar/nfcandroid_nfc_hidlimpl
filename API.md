    /**
     * Sets NFC charging feature.
     * <p>This API is for the Settings application.
     * @return True if successful
     * @hide
     */
    @SystemApi
    @FlaggedApi(Flags.FLAG_ENABLE_NFC_CHARGING)
    @RequiresPermission(android.Manifest.permission.WRITE_SECURE_SETTINGS)
    public boolean setWlcEnabled(boolean enable) {
        if (!sHasNfcWlcFeature) {
            throw new UnsupportedOperationException();
        }
        return callServiceReturn(() ->  sService.setWlcEnabled(enable), false);
    }

    /**
     * Checks NFC charging feature is enabled.
     *
     * @return True if NFC charging is enabled, false otherwise
     * @throws UnsupportedOperationException if FEATURE_NFC_CHARGING
     * is unavailable
     */
    @FlaggedApi(Flags.FLAG_ENABLE_NFC_CHARGING)
    public boolean isWlcEnabled() {
        if (!sHasNfcWlcFeature) {
            throw new UnsupportedOperationException();
        }
        return callServiceReturn(() ->  sService.isWlcEnabled(), false);

    }

    /**
     * A listener to be invoked when NFC controller always on state changes.
     * <p>Register your {@code ControllerAlwaysOnListener} implementation with {@link
     * NfcAdapter#registerWlcStateListener} and disable it with {@link
     * NfcAdapter#unregisterWlcStateListenerListener}.
     * @see #registerWlcStateListener
     * @hide
     */
    @SystemApi
    @FlaggedApi(Flags.FLAG_ENABLE_NFC_CHARGING)
    public interface WlcStateListener {
        /**
         * Called on NFC WLC state changes
         */
        void onWlcStateChanged(@NonNull WlcListenerDeviceInfo wlcListenerDeviceInfo);
    }

    /**
     * Register a {@link WlcStateListener} to listen for NFC WLC state changes
     * <p>The provided listener will be invoked by the given {@link Executor}.
     *
     * @param executor an {@link Executor} to execute given listener
     * @param listener user implementation of the {@link WlcStateListener}
     * @throws UnsupportedOperationException if FEATURE_NFC_CHARGING
     * is unavailable
     *
     * @hide
     */
    @SystemApi
    @FlaggedApi(Flags.FLAG_ENABLE_NFC_CHARGING)
    public void registerWlcStateListener(
            @NonNull @CallbackExecutor Executor executor,
            @NonNull WlcStateListener listener) {
        if (!sHasNfcWlcFeature) {
            throw new UnsupportedOperationException();
        }
        mNfcWlcStateListener.register(executor, listener);
    }

    /**
     * Unregister the specified {@link WlcStateListener}
     * <p>The same {@link WlcStateListener} object used when calling
     * {@link #registerWlcStateListener(Executor, WlcStateListener)}
     * must be used.
     *
     * <p>Listeners are automatically unregistered when application process goes away
     *
     * @param listener user implementation of the {@link WlcStateListener}a
     * @throws UnsupportedOperationException if FEATURE_NFC_CHARGING
     * is unavailable
     *
     * @hide
     */
    @SystemApi
    @FlaggedApi(Flags.FLAG_ENABLE_NFC_CHARGING)
    public void unregisterWlcStateListener(
            @NonNull WlcStateListener listener) {
        if (!sHasNfcWlcFeature) {
            throw new UnsupportedOperationException();
        }
        mNfcWlcStateListener.unregister(listener);
    }

    /**
     * Returns information on the NFC charging listener device
     *
     * @return Information on the NFC charging listener device
     * @throws UnsupportedOperationException if FEATURE_NFC_CHARGING
     * is unavailable
     */
    @FlaggedApi(Flags.FLAG_ENABLE_NFC_CHARGING)
    @Nullable
    public WlcListenerDeviceInfo getWlcListenerDeviceInfo() {
        if (!sHasNfcWlcFeature) {
            throw new UnsupportedOperationException();
        }
        return callServiceReturn(() ->  sService.getWlcListenerDeviceInfo(), null);

    }

    /**
     * Vendor NCI command success.
     * @hide
     */
    @SystemApi
    @FlaggedApi(Flags.FLAG_NFC_VENDOR_CMD)
    public static final int SEND_VENDOR_NCI_STATUS_SUCCESS = 0;
    /**
     * Vendor NCI command rejected.
     * @hide
     */
    @SystemApi
    @FlaggedApi(Flags.FLAG_NFC_VENDOR_CMD)
    public static final int SEND_VENDOR_NCI_STATUS_REJECTED = 1;
    /**
     * Vendor NCI command corrupted.
     * @hide
     */
    @SystemApi
    @FlaggedApi(Flags.FLAG_NFC_VENDOR_CMD)
    public static final int SEND_VENDOR_NCI_STATUS_MESSAGE_CORRUPTED  = 2;
