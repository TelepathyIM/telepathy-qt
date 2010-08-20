<service id="test-profile-invalid-type"
         xmlns="http://telepathy.freedesktop.org/wiki/service-profile-v1">
  <type>WrongType</type>
  <provider>TestProfileProvider</provider>
  <name>TestProfile</name>
  <icon>test-profile-icon</icon>

  <manager>testprofilecm</manager>
  <protocol>testprofileproto</protocol>
</service>
