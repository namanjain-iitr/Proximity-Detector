import React, { useState } from 'react';
import './macAddressForm.css'

const MacAddressForm = ({onAddMacAddress}) => {
  const [showForm, setShowForm] = useState(false);
  const [macAddress, setMacAddress] = useState('');
  const [name, setName] = useState('');

  const handleAddButtonClick = () => {
    setShowForm(true);
  };

  const handleCancelButtonClick = () => {
    setShowForm(false);
    setMacAddress('');
    setName('');
  };

  const handleMacAddressChange = (event) => {
      setMacAddress(event.target.value);
    };

  const handleNameChange = (event) => {
      setName(event.target.value);
    };

  const handleFormSubmit = (event) => {
    event.preventDefault();
    const macAddressRegex = /^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$/;
    if (macAddressRegex.test(macAddress)) {
      // do something with the macAddress and name, e.g. add them to a list
      onAddMacAddress(macAddress, name);
    } else {
      alert('Please enter a valid MAC address');
    }
    setMacAddress('');
    setName('');
    setShowForm(false);
  };

  return (
    <div className="mac-address-form-container">
      {showForm ? (
        <form className="mac-address-form" onSubmit={handleFormSubmit}>
        <label>
            MAC Address:
            <input className = "input-field" type="text" value={macAddress} onChange={handleMacAddressChange} required />
            </label>
            <label>
            Name:
            <input className = "input-field" type="text" value={name} onChange={handleNameChange} required />
        </label>
          <button type="submit">Add</button>
          <button type="button" onClick={handleCancelButtonClick}>
            Cancel
          </button>
        </form>
      ) : (
        <button
          className="add-mac-address-button"
          type="button"
          onClick={handleAddButtonClick}
        >
          Add MAC Address
        </button>
      )}
    </div>
  );
}

export default MacAddressForm;
